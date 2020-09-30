#include "Arduino.h"
#include <avr/pgmspace.h>

#define MY_OCR_DIVIDER 2 // Base trigger of output compare register, i.e. every N increments of Timer1 do the interrupt routine with speed/accel/step evaluation
// if it's too low, it'll cause interrupt overlaps and skips - experiment with this and the prescaler value in the interrupt definition!
// also the slowdown logic needs to be scaled to match the frequency of steps ...
#define MIN_SAFE_SPEED 3 // never below 2! Experiment with this vs the ocr_divider, it sets the maximum step speed by defining a minimum amount of delay loops; if the motor stalls it's too low

#define NUM_AXIS 3 // how many motors to drive
#define STEPS_LIN 200
#define STEPS_ROT 100

/* motion related variables */

#define numOfPoints 2 // number of storable positions: 0 = start, 1 = end (for now)
byte modus;           // overall operation mode
extern byte modus;

uint16_t posCursor; // points to currently active positions[] value
extern uint16_t posCursor;

/* arrays of positions and pins */

long positions[numOfPoints][NUM_AXIS];        // list of positions
extern long positions[numOfPoints][NUM_AXIS]; // make global

long volatile currentpos[NUM_AXIS];        // current position for each axis
extern long volatile currentpos[NUM_AXIS]; // make global

long volatile targetpos[NUM_AXIS];
extern long volatile targetpos[NUM_AXIS]; // make global

uint8_t pins_dir[] = {7, 9, 11};
extern uint8_t pins_dir[]; // make global

uint8_t pins_step[] = {8, 10, 12};
extern uint8_t pins_step[]; // make global

uint8_t outPins[] = {7, 8, 9, 10, 11, 12};
extern uint8_t outPins[]; // make global

/* analog input / joystick control */

#define MY_STICK_USED 3  // how many analog inputs to use
#define MY_STICK_DEAD 64 // size of the no-motion dead zone around the center; 7 % left/right

uint8_t stickPins[] = {A2, A1, A0}; // Mapping of inputs to Axis X, Y, Z
extern uint8_t stickPins[];         // make global

int stickInputRaw[MY_STICK_USED];  // storage for raw analog values read
extern int stickInputRaw[];        // make global
int stickStepsize[MY_STICK_USED];  // storage for processed/limited values
extern int stickStepsize[];        // make global
int stickDirection[MY_STICK_USED]; // storage for direction (duh)
extern int stickDirection[];       // make global
int stickCenter[MY_STICK_USED];    // storage for center reference
extern int stickCenter[];          // make global
int stickBresenham[MY_STICK_USED]; // storage for Bresenham's line draw algorithm
extern int stickBresenham[];       // make global
int stickMax[MY_STICK_USED];       // the smaller of the distance values as calculate from 0 -- CENTER -- 1023
extern int stickMax[];             // make global

/* timing variables */

unsigned long lastShow = 0;    // timer
extern unsigned long lastShow; // used globally

uint16_t volatile stepDelay = 10;   // the bigger the slower the sled moves. User-adjusted value
extern uint16_t volatile stepDelay; // used globally

uint16_t volatile stepDelayStep = 1;    // used for convenience, can cycle through 1,10,10,1000,10000 with
extern uint16_t volatile stepDelayStep; // used globally

unsigned long theTick = 0;    // millis() buffer
extern unsigned long theTick; // used globally

/* texts */

char strbuf[17];      // LCD lineout buffer; 16 chars + x00
extern char strbuf[]; // used globally

char txt_stop[] = "****  STOP  ****";
char txt_moving[] = "**** MOVING ****";

uint16_t volatile tickerNow;        // internal counter, increments in interrupt, resets as it reaches curTicker. At ticker = 0 the STEP is pulled high, at ticker = 1 it's pulled low again
extern uint16_t volatile tickerNow; // declare global

uint16_t volatile tickerLimit = 10;   // actual used step delay value of 0-65535, 0 = no delay; 65535 = very long delay
extern uint16_t volatile tickerLimit; // declare global

bool volatile enblStep;        // a flag to short-circuit the interrupt, stops the sled immediately
extern bool volatile enblStep; // declare global

bool volatile targetReached = false; // the interrupt routine sets this to true once curPos = targPos
extern bool volatile targetReached;  // declare global

/*
Bresenham line plot algorithm data
*/
long volatile steps[NUM_AXIS];
extern long volatile steps[NUM_AXIS]; // make global

long volatile steps_dir[NUM_AXIS];
extern long volatile steps_dir[NUM_AXIS]; // make global

long volatile bresenham[NUM_AXIS];
extern long volatile bresenham[NUM_AXIS]; // make global

long volatile stepsmax, stepstodo;
extern long volatile stepsmax, stepstodo; // make global

/* input pins */

#define PIN_ROTA_1 5
#define PIN_ROTA_2 6

#define PUSH_CYCLE 4   // mode cycle
#define PUSH_SKIP 2    // skip 2 next
#define PUSH_STASTOP 3 // start/stop

byte pullPins[] = {
    PIN_ROTA_1, PIN_ROTA_2, PUSH_CYCLE, PUSH_SKIP, PUSH_STASTOP}; // these will get INPUT_PULLUP

/* Buttons A-E debouncer output - each state will be set for ONE LOOP after calling multiDebouncer() then revert to 0 */

byte buttonState[] = {0, 0, 0}; // Buttons 0-2
#define BUTTONSTATE_CYCLE 0     // offset in array
#define BUTTONSTATE_SKIP 1      // offset in array
#define BUTTONSTATE_STASTOP 2   // offset in array
#define BUTTON_TIME_LONG 1000   // 1000 ms or longer counts as "long press"
#define BUTTON_PRESS_NONE 0     // button is not pressed at all
#define BUTTON_PRESS_SHORT 1    // button was released within 1 seconds of press detection during last check
#define BUTTON_PRESS_LONG 2     // button was released AFTER 1+ second of press detection during last check

/* general operation mode stuff */

#define DIR_UP true
#define DIR_DOWN false

#define MODE_STOP 0    // no movement
#define MODE_RUN 2     // Mode: normal run with adjustable delay
#define MODE_PREVIEW 3 // set start and end position interactively
#define MODE_BRAKE 4   // after stop was pushed, wait for slowdown period

#define ACCEL_SPEEDUP 2           // reduce the slowDown value each interrupt until it is zero
#define ACCEL_SLOWDOWN 1          // increase the slowDown value each interrupt until it is MOVE_MAX_SLOWDOWN
#define ACCEL_NEUTRAL 0           // do nothing with slowDown
#define ACCEL_MAX_SLOWDOWN 20 * 8 // capping the additional slowDown per step (effective slowdown will increase every 3 ints)
#define SLOWDOWN_THRESHOLD 30     // if stepDelay is lower than this, we need to ease in/out

#define SHOW_EVERY_N_MILLIS 333 // i2c LCD output is updated at most this often

/* motion patterns */

byte motionPattern = 0;
PROGMEM const char motionPatternTexts[][12] = { 
    "   PINGPONG",
    "ONEWAY TRIG",
    "ONEWAY REPE",
    "STICK DRIVE"};
#define MOPA_PINGPONG 0      // default behavior: run back and forth between A and B at preset speed
#define MOPA_ONEWAY 1        // runs from A to B with the preset speed, waits for button, then returns to A at max speed, waits again
#define MOPA_ONEWAY_REPEAT 2 // runs to position B with the preset speed, returns to A at max speed, repeats with no interaction
#define MOPA_STICK 3         // runs to position B with the preset speed, returns to A at max speed, repeats with no interaction

/* Main Menu selectable options */

PROGMEM const char mainmenu_texts[][17] = {
    "Set Start Pos   ",
    "Set End Pos     ",
    "Set Motion Mode ",
    "Set Step Delay  ",
    "Options         "};

byte mainmenu_option = 0;

#define mainmenu_MAX 4 // 0-4

/* Active Panel Mode */


#define PM_STARTSET 0
#define PM_ENDSET 1
#define PM_MOPASET 2
#define PM_DELAYSET 3
#define PM_OPTIONS 4
#define PM_MAINMENU 5 // Init point, select menu points by rotation knob

byte panelMode = 5;

PROGMEM const char romTexts[][17] = { 
    "Exit < T0 > SET ", // 0
    "Exit < T1 > SET ", // 1
    "CamSlider  ready", // 2
    "< Turn > or Push", // 3
    "Stick Center Cal", // 4
    "RELEASE JOYSTICK", // 5
    "Calib start in  ", // 6
    "Push to select  ", // 7
    };
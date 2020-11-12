#include "Arduino.h"

volatile struct motor_pin motor_pins[NUM_AXIS] = {
    {25, 26, LOW, HIGH},
    {27, 14, LOW, HIGH},
    {12, 13, LOW, HIGH}};
extern volatile motor_pin motor_pins[]; // make global

uint8_t outPins[] = {25, 26, 27, 14, 12, 13};
extern uint8_t outPins[]; // make global

uint8_t stickPins[] = {34, 35, 32}; // Mapping of inputs to Axis X, Y, Z
extern uint8_t stickPins[];         // make global

#define ANALOG_RANGE 3750 // measured, adjust as you see fit
#define MY_STICK_DEAD 160 // guestimate size of the no-motion deadband zone around the center; 

/* input pins */

#define PIN_ROTA_1 17
#define PIN_ROTA_2 16

#define PUSH_ENCODER 15   // encoder push button
#define PUSH_SKIP 5    // skip to next target
#define PUSH_STASTOP 33 // start/stop

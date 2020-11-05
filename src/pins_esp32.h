#include "Arduino.h"

volatile struct motor_pin motor_pins[NUM_AXIS] = {
    {25, 33, LOW, HIGH},
    {27, 26, LOW, HIGH},
    {12, 14, LOW, HIGH}};
extern volatile motor_pin motor_pins[]; // make global

uint8_t outPins[] = {33, 25, 26, 27, 14, 12};
extern uint8_t outPins[]; // make global

uint8_t stickPins[] = {34, 35, 32}; // Mapping of inputs to Axis X, Y, Z
extern uint8_t stickPins[];         // make global

#define ANALOG_RANGE 4096

/* input pins */

#define PIN_ROTA_1 16
#define PIN_ROTA_2 17

#define PUSH_ENCODER 15   // encoder push button
#define PUSH_SKIP 5    // skip to next target
#define PUSH_STASTOP 33 // start/stop

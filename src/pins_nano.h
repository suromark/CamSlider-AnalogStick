#include "Arduino.h"

volatile struct motor_pin motor_pins[NUM_AXIS] = {
    {8, 7, LOW, HIGH},
    {10, 9, LOW, HIGH},
    {12, 11, LOW, HIGH}};
extern volatile motor_pin motor_pins[]; // make global

uint8_t outPins[] = {7, 8, 9, 10, 11, 12};
extern uint8_t outPins[]; // make global

uint8_t stickPins[] = {A1, A2, A0}; // Mapping of inputs to Axis X, Y, Z
extern uint8_t stickPins[];         // make global

#define ANALOG_RANGE 1024
#define MY_STICK_DEAD 64 // size of the no-motion deadband zone around the center; 

/* input pins */

#define PIN_ROTA_1 5
#define PIN_ROTA_2 6

#define PUSH_ENCODER 4   // encoder push button
#define PUSH_SKIP 2    // skip to next target
#define PUSH_STASTOP 3 // start/stop

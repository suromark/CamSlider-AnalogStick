#include "Arduino.h"

volatile struct motor_pin motor_pins[NUM_AXIS] = {
    {33, 25, LOW, HIGH},
    {26, 27, LOW, HIGH},
    {14, 12, LOW, HIGH}};
extern volatile motor_pin motor_pins[]; // make global

uint8_t outPins[] = {33, 25, 26, 27, 14, 12};
extern uint8_t outPins[]; // make global

uint8_t stickPins[] = {34, 35, 32}; // Mapping of inputs to Axis X, Y, Z
extern uint8_t stickPins[];         // make global

#include "Arduino.h"

volatile struct motor_pin motor_pins[NUM_AXIS] = {
    {7, 8, LOW, HIGH},
    {9, 10, LOW, HIGH},
    {11, 12, LOW, HIGH}};
extern volatile motor_pin motor_pins[]; // make global

uint8_t outPins[] = {7, 8, 9, 10, 11, 12};
extern uint8_t outPins[]; // make global

uint8_t stickPins[] = {A1, A2, A0}; // Mapping of inputs to Axis X, Y, Z
extern uint8_t stickPins[];         // make global


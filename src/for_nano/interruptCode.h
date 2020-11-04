#ifndef interruptCode_h
#define interruptCode_h

#include <Arduino.h>

#define INTERRUPTS_PER_SECOND 20833 // theoretical should be 31250, but stopwatch says it's 66 % of that ...?

void oneCycle() {
    // nothing here, only used on ESP32
}

void setUpInterruptForNano()
{

    /* 

init our main interrupt action, based on documentation: 
https://www.heise.de/developer/artikel/Timer-Counter-und-Interrupts-3273309.html
e.g. dividerCount = desiredDeltaT * cpufreq / prescale = 0.0001 * 16.000.000 / 256 = 62
30 = X * 16.000.000 / 8 -> X = 0,015 ms -> 66666 interrupts / second
 */

    // Timer 1 set to prescaler = 1/256, trigger at every MY_OCR_DIVIDER impulse
    // interrupt speed @ CPU 16 MHz is 16 M / 2 / 256 = 31250 calls/second
    // note: we do a step at most every 3rd call ( MIN_SAFE_SPEED ) = top speed is 10416 steps/second

    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;                                         // Register init with 0
    OCR1A = MY_OCR_DIVIDER;                            // Output Compare Register - base speed
    TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10); // Clock Prescaler: 101 = 1024, 100 = 256, 011 = 64, 010 = 8; 000 = no prescale
    TIMSK1 |= (1 << OCIE1A);                           // Timer Compare Interrupt enable
    interrupts();
};

/**
 * 
 * 
 * 
 * Interrupt routine
 */
ISR(TIMER1_COMPA_vect)
{
    TCNT1 = 0; // Register re-init with speed value (necessary?)

    static uint16_t closingin = 0;   // extra delay on final approach
    static uint16_t takingoff = 256; // extra delay during first steps

    if (enblStep == false) /* motor is disabled, abort early */
    {
        takingoff = 256; // if the motor is on hold, prepare soft start
        return;
    }

    tickerNow++; // increase the internal subdivider;

    if (tickerNow == 1)
    {
        for (int i = 0; i < NUM_AXIS; i++)
        {
            digitalWrite(motor_pins[i].pin_step, LOW); // always pull down Step signal after 2 timer loop
        }
    }

    /* Do nothing if target is reached */

    if (stepstodo == 0)
    {
        enblStep = false;
        targetReached = true;
        return;
    }
    else
    {
        targetReached = false;
    }

    /*
    */

    /*
        Time for another step calculation?
        */

    if (tickerNow >= MIN_SAFE_SPEED + tickerLimit + (closingin >> 1) + (takingoff >> 5))
    {

        tickerNow = 0; // restart the counter

        if (takingoff > 0)
        {
            takingoff--;
        }

        /* calculate the closingin additional delay 
    for the next call as "the closer to target, the slower" 
    - works for both accel and decel, 
    and the bigger tickerLimit gets (slower motion), 
    the less closingin affects the speed */

        if (stepstodo < 256)
        {
            closingin = (256 - stepstodo) >> 4;
        }
        else
        {
            closingin = 0;
        }

        /* target reached? */

        if (stepstodo-- == 0)
        {
            enblStep = false;
            targetReached = true;
            return;
        }

        /* do the Bresenham */

        for (int i = 0; i < NUM_AXIS; i++)
        {
            bresenham[i] -= steps[i];
            if (bresenham[i] < 0) // time for a step
            {
                bresenham[i] += stepsmax;                   // re-prime while considering underflow
                currentpos[i] += steps_dir[i];              // the position change
                digitalWrite(motor_pins[i].pin_step, HIGH); // the actual step signal
            }
        }
    }
    return;
};

#endif
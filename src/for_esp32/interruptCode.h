#ifndef interruptCode_h
#define interruptCode_h

#include <Arduino.h>

void setUpInterruptForESP32()
{
    // we're faking it with the oneCycle() call
}

void oneCycle()
{
    static uint16_t closingin = 0;   // extra delay on final approach
    static uint16_t takingoff = 256; // extra delay during first steps
    static unsigned long lastMicros = 0;
    static unsigned long nowMicros = 0;

    nowMicros = micros();

    if( lastMicros + 1000 > nowMicros ) {
        return; // not yet time!
    }

    lastMicros = nowMicros;

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
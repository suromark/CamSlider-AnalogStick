#ifndef interruptCode_h
#define interruptCode_h

#define MIN_SAFE_SPEED 3 // never below 2! Experiment with this vs the ocr_divider, it sets the maximum step speed by defining a minimum amount of delay loops; if the motor stalls it's too low

#include <Arduino.h>

void IRAM_ATTR oneCycle();

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void setUpInterruptForESP32()
{
    timer = timerBegin(0, 800, true); // 80 Mhz downscale to 100,000 ticks/second
    timerAttachInterrupt(timer, &oneCycle, true);
    timerAlarmWrite(timer, 10, true); // call every 10th timer step = 10k /second
    timerAlarmEnable(timer);
}

void IRAM_ATTR oneCycle()
{
    static uint16_t closingin = 0;   // extra delay on final approach
    static uint16_t takingoff = 256; // extra delay during first steps

    portENTER_CRITICAL_ISR(&timerMux); // ideally wrap the interrupt routine in this

    do
    {

        if (enblStep == false) /* motor is disabled, abort early */
        {
            takingoff = 256; // if the motor is on hold, prepare soft start
            break;
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
            break;
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
                break;
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
    } while (0);

    portEXIT_CRITICAL_ISR(&timerMux);

    return;
};

#endif
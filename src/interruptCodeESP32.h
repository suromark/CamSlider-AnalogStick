#ifndef interruptCode_h
#define interruptCode_h

#define MIN_SAFE_SPEED 7
// never below 2! Experiment with this vs the interrupt prescalers,
// it sets the maximum step speed by defining a minimum amount of delay loops
// if the motor stalls or skips at top speed then you know it's too low
// this defines the minimum amount of interrupts required before another step is taken
// since at tickerNow = 0 -> raise step signal,
// tickerNow = 1 -> clear step signal,
// tickerNow = 2 ... -> wait for delay to end, then reset tickerNow to zero and start over

#define INTERRUPTS_PER_SECOND 50000
// this is for calculating the "remaining time" display;

#include <Arduino.h>

void IRAM_ATTR oneCycle();

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

/*
gets called during setup()
*/
void setUpInterruptForESP32()
{
    pinMode(2, OUTPUT);              // onboard LED pulsing
    timer = timerBegin(0, 80, true); // 80 Mhz downscale to 1 Megaticks/second
    timerAttachInterrupt(timer, &oneCycle, true);
    timerAlarmWrite(timer, 20, true); // call every N-th timer step, 50k /second
    timerAlarmEnable(timer);
}

long platform_specific_forecast()
{
    /* calculate remaining runtime from remaining steps / steps per second  */
    return (MIN_SAFE_SPEED + tickerLimit) * stepstodo / INTERRUPTS_PER_SECOND;
}

void IRAM_ATTR oneCycle()
{
    static uint16_t closingin = 0;   // extra delay on final approach
    static uint16_t takingoff = 256; // extra delay during first steps

    portENTER_CRITICAL_ISR(&timerMux); // ideally wrap the interrupt routine in this

    digitalWrite(2, HIGH);

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
                if (manualOverride[i])
                {
                    continue;
                }

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

        if (tickerNow >= MIN_SAFE_SPEED + tickerLimit + (closingin >> 1) + (takingoff >> 4))
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
                if (manualOverride[i])
                {
                    continue;
                }

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

    digitalWrite(2, LOW);

    portEXIT_CRITICAL_ISR(&timerMux);

    return;
};

#endif
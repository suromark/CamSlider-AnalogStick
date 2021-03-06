#define FILE_INFO "Source: CamSlider-AnalogStick - env: platformio"
// #include "myi2c.h"
#include "globals.h"

#include "myLCD.h"
myLCD lcd;

/* functions */

void initDebug();
void runNormal();
void runBrake();
void startFromHalt();
void showPositionAndTarget();
void printMoveDiagnostic(bool theFlag);
void mopa_pingpong();      // an automated cyclic motion behaviour between position marks 0 and 1
void mopa_oneway_trig();   // a button-triggered motion: slow from 0 to 1, button, fast from 1 to 0
void mopa_oneway_repeat(); // an automated cyclic motion: mark0 - slow - mark1 - fast - mark0 - repeat
void mopa_stick();         // drive by wire, use analog inputs to do timed steps depending on offset from center
void checkEncoder();
void turnUp();
void turnDown();
void smartDelayStep();
void multiDebouncer();
void doButtonInput();
void mainmenu_previousoption();
void mainmenu_nextoption();
void disp_panelmode();
byte setTargetToPositionItem(int index); // copy entry $index of positions[] into targetpos, return TRUE if target != position
void setTargetForBrake();                // shifts targetPosition so that the motor can spin down; avoids immediate motion flipping
void gotoNextTarget();                   // increases / cycles the target cursor and declares the new position; then starts the motor
void newPreview(int previewIndes);       // used for preview; quick chase a target position that may vary interactively at any time
void recalculateMotion();
void recalculateDelay();
void currentPosToStorage(byte cursor);
void reportPositionToSerial();

/* device-specific */

#ifdef ARDUINO_AVR_NANO
#include "interruptCodeNano.h"
#endif

#ifdef ARDUINO_NodeMCU_32S
#include "interruptCodeESP32.h"
#include "extra_esp32.h"
#endif


#define countof(a) (sizeof(a) / sizeof(a[0])) // matrix rows calculator

void setup()
{
    enblStep = false;
    Serial.begin(115200);

    Serial.println(FILE_INFO);
    Serial.print("Compiled ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);

    if (!lcd.setup(0x27))
    {                    // all my previous 1602 I2C bridge chips use this
        lcd.setup(0x3f); // but on mk2 I found some display uses this one
    }

    // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
    snprintf(strbuf, sizeof strbuf, "%S", romTexts[5]);
    lcd.print(0, 0, strbuf);

    for (int i = 4; i > 0; i--)
    {
        Serial.println(i);
        // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
        snprintf(strbuf, sizeof strbuf, "%.14S %1u", romTexts[6], i);
        lcd.print(1, 0, strbuf);
        lcd.lighton();
        delay(800);
        lcd.lightoff();
        delay(200);
    }
    lcd.lighton();

    for (byte i = 0; i < sizeof outPins; i++)
    {
        pinMode(outPins[i], OUTPUT);
    }

    for (byte i = 0; i < sizeof pullPins; i++)
    {
        pinMode(pullPins[i], INPUT_PULLUP);
    }

    for (int i = 0; i < NUM_AXIS; i++)
    {
        currentpos[i] = 0;     // startpos = 0
        manualOverride[i] = 0; // no manual override on boot
    }

    /* calibrate analog input center (assumes that on power-up stick will be at center) */

    for (int i = 0; i < MY_STICK_USED; i++)
    {

        int c = 0;
        int r = 0;

        for (int j = 0; j < 16; j++)
        {
            r = analogRead(stickPins[i]);
            c += r;
            // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
            snprintf(strbuf, sizeof strbuf, "%S", romTexts[4]);
            lcd.print(0, 0, strbuf);
            snprintf(strbuf, sizeof strbuf, "Axis %1u/%2u V=%4u", i, j, r);
            lcd.print(1, 0, strbuf);
        }

        stickCenter[i] = c / 16;

        if (stickCenter[i] < 512)
        {
            stickMax[i] = stickCenter[i] - MY_STICK_DEAD;
        }
        else
        {
            stickMax[i] = ANALOG_RANGE - stickCenter[i] - MY_STICK_DEAD;
        }

        Serial.print("Axis ");
        Serial.print(i);
        Serial.print(" Center at ");
        Serial.print(stickCenter[i]);
        Serial.print(" Max Range ");
        Serial.print(stickMax[i]);

        Serial.println("");

        stickBresenham[i] = stickMax[i];
    }

    /* set some default values */

    stepDelay = 60;
    tickerLimit = stepDelay;
    tickerNow = 0;
    mainMode = MODE_STOP;
    motionPatternSelect = MOPA_STICK;
    motionPatternActive = MOPA_STICK;

    /* clear coordinates list */

    for (int j = 0; j < NUM_AXIS; j++)
    {

        for (int i = 0; i < numOfPoints; i++)
        {
            positions[i][j] = 0;
        }
    }
    // Display start

    lcd.clear();
    // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
    snprintf(strbuf, sizeof strbuf, "%S", romTexts[3]);
    lcd.print(1, 0, strbuf);

#ifdef ARDUINO_AVR_NANO

    // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
    snprintf(strbuf, sizeof strbuf, "%S", romTexts[2]);
    lcd.print(0, 0, strbuf);

    setUpInterruptForNano();
#endif

#ifdef ARDUINO_NodeMCU_32S

    // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
    snprintf(strbuf, sizeof strbuf, "%S", romTexts[10]);
    lcd.print(0, 0, strbuf);

    setupExtra();

    setUpInterruptForESP32();
#endif

    if (false)
    {
        Serial.println(F("Debug Mode"));
        initDebug();
    }

    Serial.println(F("Setup done."));
}

/* ##########################################################################################################






 */

void loop()
{

    theTick = millis();

    multiDebouncer();

    checkEncoder();

    doButtonInput();

#ifdef ARDUINO_NodeMCU_32S
    loopExtra();
#endif

    switch (mainMode)
    {
    case MODE_RUN:
        runNormal();
        break;

    case MODE_BRAKE:
        runBrake();
        /* stop was pushed, wait for slowdown to end */
        break;

    case MODE_PREVIEW:
        /* control/interaction happens inside events of checkEncoder() */
        break;

    default:
        break;
    }
}

void reportPositionToSerial()
{
    for (byte i = 0; i < NUM_AXIS; i++)
    {
        Serial.print(F("AXIS "));
        Serial.print(i);
        Serial.print(F(" = "));
        Serial.println(currentpos[i]);
    }
}

/* ##########################################################################################################









*/
void setTargetForBrake()
{
    /* Should set an immediate target position near the current one to trigger brake/stop */

    long slowdown = 0;

    if (stepDelay < 20) // moving fast, need extra length for slowdown
    {
        slowdown = 120 - (6 * stepDelay);
    }
    else
    {
        slowdown = 0; // at that speed we don't need slowdown anyway
    }

    if (stepstodo > slowdown) // do we have longer than needed?
    {
        stepstodo = slowdown;
    }
}
/* ##########################################################################################################





The faster the button is turned, the bigger the step becomes
*/
void smartDelayStep()
{

    static unsigned long lastTurnPulse = 0;

    if (lastTurnPulse + 150 < theTick)
    {
        stepDelayStep = 1;
    }
    else if (lastTurnPulse + 80 < theTick)
    {
        stepDelayStep = 10;
    }
    else if (lastTurnPulse + 50 < theTick)
    {
        stepDelayStep = 100;
    }
    else
    {
        stepDelayStep = 1000;
    }

    lastTurnPulse = theTick;
}
/* ##########################################################################################################







wait until brake phase ended, then go to stop phase
and calculate the remaining steps to the current target
(for time calculation to work, so stepstodo is not zero)
*/

void runBrake()
{
    if (targetReached)
    {
        mainMode = MODE_STOP;
        setTargetToPositionItem(posCursor);
        recalculateMotion();
        panelMode = PM_DELAYSET;
    }
}

/* ##########################################################################################################










Responses to button presses
*/
void doButtonInput()
{

    // Encoder button activates the currently selected main menu item, or returns to main menu

    if (buttonState[BUTTONSTATE_ENCODER] == BUTTON_PRESS_SHORT)
    {
        switch (panelMode)
        {

        case PM_MOPASET:
            motionPatternActive = motionPatternSelect;
            panelMode = PM_CONFIRMMODE;
            break;

        case PM_MAINMENU:

            switch (mainmenu_option)
            {
            case PM_STARTSET:
                currentPosToStorage(0);
                recalculateMotion();
                panelMode = PM_CONFIRMPOS;
                break;

            case PM_ENDSET:
                currentPosToStorage(1);
                recalculateMotion();
                panelMode = PM_CONFIRMPOS;
                break;

            default:
                panelMode = mainmenu_option;
                break;
            }
            break;

        default:
            panelMode = PM_MAINMENU;
            break;
        }

        disp_panelmode();
    }

    // Start-Stop does start-stop

    if (buttonState[BUTTONSTATE_STASTOP] != BUTTON_PRESS_NONE)
    {

        if (mainMode == MODE_RUN)
        { // STOP has priority
            setTargetForBrake();
            mainMode = MODE_BRAKE;
            Serial.println(F("Disable Motion"));
            reportPositionToSerial();
            // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
            snprintf(strbuf, sizeof strbuf, "%S", romTexts[11]); // stop
            lcd.print(0, 0, strbuf);
            return;
        }
        else
        {
            recalculateDelay();
            recalculateMotion();
            Serial.println(F("Enable Motion"));
            reportPositionToSerial();
            startFromHalt();
            mainMode = MODE_RUN;
            // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
            snprintf(strbuf, sizeof strbuf, "%S", romTexts[12]); // moving
            lcd.print(0, 0, strbuf);
        }
    }

    // Extra button sets the posCursor to its next entry
}
/* #####################################################################################################






cycle through main menu items upwards
 */

void mainmenu_nextoption()
{

    if (++mainmenu_option > mainmenu_MAX)
    {
        mainmenu_option = 0;
    }
}
/* #####################################################################################################






Cycle through main menu items downward
 */

void mainmenu_previousoption()
{
    if (mainmenu_option-- == 0)
    {
        mainmenu_option = mainmenu_MAX;
    }
}
/* #####################################################################################################






Debugging: pingpong all motor channels
 */

void initDebug()
{

    for (int j = 0; j < NUM_AXIS; j++)
    {
        positions[1][j] = 1000;
    }
    mainMode = MODE_RUN;
    motionPatternActive = MOPA_PINGPONG;

    startFromHalt();
}

/* #####################################################################################################









Standard loop behaviour
 */
void runNormal()
{
    static unsigned long nextShow = 0;

    switch (motionPatternActive)
    {
    case MOPA_ONEWAY:
        mopa_oneway_trig();
        break;

    case MOPA_ONEWAY_REPEAT:
        mopa_oneway_repeat();
        break;

    case MOPA_STICK:
    case MOPA_STICK_SLOW:
        mopa_stick();
        break;

    default:
        mopa_pingpong();
        break;
    }

    if (theTick > nextShow)
    {
        if (motionPatternActive == MOPA_STICK || motionPatternActive == MOPA_STICK_SLOW)
        {
            // stick drive does not update the display for performance reasons
        }
        else
        {
            showPositionAndTarget();
        }
        nextShow = theTick + SHOW_EVERY_N_MILLIS;
    }
}

/* #####################################################################################################







Motion pattern driven by 3-axis joystick input
 */
void mopa_stick()
{
    static byte loopa = 0; // internal helper
    static unsigned long nextMicroTick = 0;
    static unsigned long nowMicroTick = 0;

    enblStep = false; // we do drive logic ourselves, so keep the interrupt disabled

    nowMicroTick = micros();

    if (nextMicroTick > nowMicroTick)
    {
        return;
    }

    nextMicroTick = nowMicroTick + 100 + (motionPatternActive == MOPA_STICK_SLOW ? 1350 : 0); // this is more precise than the coarse millis()

    // always clear the steps pins

    for (byte i = 0; i < MY_STICK_USED; i++)
    {
        digitalWrite(motor_pins[i].pin_step, 0);
    }

    // read one input per loop only, to even out the required extra time for analogRead() ( about 1/10,000 sec.)

    if (++loopa >= MY_STICK_USED)
    {
        loopa = 0;
    }

    stickInputRaw[loopa] = analogRead(stickPins[loopa]);

    if (stickInputRaw[loopa] < stickCenter[loopa])
    {
        stickStepsize[loopa] = stickCenter[loopa] - stickInputRaw[loopa];
        stickDirection[loopa] = -1;
    }
    else
    {
        stickStepsize[loopa] = stickInputRaw[loopa] - stickCenter[loopa];
        stickDirection[loopa] = 1;
    }

    if (stickStepsize[loopa] > stickMax[loopa])
    {
        stickStepsize[loopa] = stickMax[loopa];
    }

    if (stickStepsize[loopa] < MY_STICK_DEAD)
    {
        stickStepsize[loopa] = 0; // no move inside dead zone
    }
    else
    {
        stickStepsize[loopa] -= MY_STICK_DEAD; // step size outside dead zone
    }

    if (0) // for debugging only, disable for speed
    {
        Serial.print(loopa);
        Serial.print(" SS=");
        Serial.print(stickStepsize[loopa]);
        Serial.print(" SD=");
        Serial.print(stickDirection[loopa]);
        Serial.print(" RAW=");
        Serial.print(stickInputRaw[loopa]);
        Serial.print(" CAL=");
        Serial.print(stickCenter[loopa]);
        Serial.println("");
    }

    // Bresenham all channels to decide if step pulse required

    for (byte i = 0; i < MY_STICK_USED; i++)
    {
        stickBresenham[i] -= stickStepsize[i];
        if (stickBresenham[i] < 0) // time for a step
        {
            stickBresenham[i] += stickMax[i];   // re-prime while considering underflow
            currentpos[i] += stickDirection[i]; // the position change
            digitalWrite(motor_pins[i].pin_dir, (stickDirection[i] > 0 ? motor_pins[i].lvl_forw : motor_pins[i].lvl_back));
            digitalWrite(motor_pins[i].pin_step, HIGH); // the actual step signal
        }
    }
}
/* #####################################################################################################







Motion pattern back and forth at constant slow speed
 */
void mopa_pingpong()
{

    static bool nextafterstop = false;

    /* reach/pong behavior */

    if (targetReached == true)
    {
        nextafterstop = false;
        gotoNextTarget();
        return;
    }

    /* responding to button B */
    if (buttonState[BUTTONSTATE_SKIP] == BUTTON_PRESS_SHORT)
    {
        if (targetReached == false) /* we're mid-move, do slow down first! */
        {
            setTargetForBrake();
        }
        else
        {
            gotoNextTarget();
        }
    }
}

/* #####################################################################################################



Motion pattern "wait for B, slow to point 1, wait for B, fast to point 0"
*/
void mopa_oneway_trig()
{
    static bool toldyou = false;
    static bool nextafterstop = false;

    if (targetReached == true && nextafterstop == true)
    {
        nextafterstop = false;
        toldyou = false;
        gotoNextTarget();
    }

    /* responding to button B */
    if (buttonState[BUTTONSTATE_SKIP] == BUTTON_PRESS_SHORT)
    {
        toldyou = false;

        if (targetReached == false) /* we're mid-move, do slow down first! */
        {
            nextafterstop = true;
            setTargetForBrake();
        }
        else
        { /* we're standing still at endpoint, so go ahead directly */
            nextafterstop = false;
            gotoNextTarget();
        }
    }

    if (toldyou == false) // tell user (usually at endpos only to avoid delays)
    {
        toldyou = true;
        if (posCursor == 0)
        {
            // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
            snprintf(strbuf, sizeof strbuf, "Start %c  Press B", 3);
        }
        else
        {
            // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
            snprintf(strbuf, sizeof strbuf, "Return %c Press B", 3);
        }
        lcd.print(1, 0, strbuf);
    }
}
/* #####################################################################################################






*/
void gotoNextTarget()
{
    enblStep = false;
    posCursor++;
    if (posCursor >= numOfPoints)
    {
        posCursor = 0;
    }

    if (setTargetToPositionItem(posCursor))
    {
        Serial.print(F("Moving to new target item "));
        Serial.println(posCursor);

        recalculateDelay();
        recalculateMotion();
        startFromHalt();
    }
}

/* #####################################################################################################




Motion Pattern "slow to point 1, fast return to point 0, repeat automatically"
*/
void mopa_oneway_repeat()
{

    /* press button B to skip to the next target position */

    if (buttonState[BUTTONSTATE_SKIP] == BUTTON_PRESS_SHORT)
    {
        setTargetForBrake();
    }

    /* reach/pong behavior */

    if (targetReached)
    {
        gotoNextTarget();
    }
}

/* 
#####################################################################################################





 */
void startFromHalt()
{
    targetReached = false;
    enblStep = true;
}
/*
#####################################################################################################






*/
void newPreview(int previewIndex)
{
    mainMode = MODE_PREVIEW;

    if (setTargetToPositionItem(previewIndex))
    {
        recalculateDelay();
        recalculateMotion();
        posCursor = previewIndex;
        startFromHalt();
    }
}
/*
#####################################################################################################







Schreibe den Eintrag positions[index] in die targetpos
Liefere TRUE wenn das Ziel von der momentanen Position abweicht, sonst FALSE
*/
byte setTargetToPositionItem(int index)
{
    byte mustMove = false;

    for (int i = 0; i < NUM_AXIS; i++)
    {
        if (targetpos[i] != positions[index][i])
        {
            mustMove = true;
            targetpos[i] = positions[index][i];
        }
    }
    return mustMove;
}

/*
#####################################################################################################








*/
void gotoTarget()
{
    /* no special action required here */
}
/* #####################################################################################################










Put position/target info to LCD
 */
void showPositionAndTarget()
{
    long remainingSeconds = -1;
    char uhrzeit[7];

    remainingSeconds = platform_specific_forecast();

    if (remainingSeconds > 5999940)
    {
        strncpy(uhrzeit, "++++++", 6); // time overflow
    }
    else if (remainingSeconds > 5999)
    {
        // show minutes only
        snprintf(uhrzeit, sizeof uhrzeit, "%5ldm", remainingSeconds / 60);
    }
    else if (remainingSeconds > 60)
    {
        // show minutes:seconds
        snprintf(uhrzeit, sizeof uhrzeit, "%2ldm%2lds", remainingSeconds / 60, remainingSeconds % 60);
    }
    else
    {
        // show seconds only
        snprintf(uhrzeit, sizeof uhrzeit, "%5lds", remainingSeconds);
    }

    //    snprintf(strbuf, sizeof strbuf, "%6ld%c%6ld T%1u ", currentpos[0], 3, targetpos[0], posCursor); // string longer to clear rest of line (buffer = 16 usable chars only)
    snprintf(strbuf, sizeof strbuf, "%.6s %6ld%cT%1u", uhrzeit, stepstodo, 3, posCursor); // string longer to clear rest of line (buffer = 16 usable chars only)
    lcd.print(0, 0, strbuf);
}

/* #####################################################################################################




*/

void printMoveDiagnostic(bool theFlag)
{
    return; // nah, let's save some cycles
    Serial.print(F("Current delay "));
    // Serial.println(tickerXLimit);
    if (theFlag == DIR_UP)
    {
        Serial.print(F("MaxPosition touched at "));
    }
    else
    {
        Serial.print(F("MinPosition touched at "));
    }
    for (int i = 0; i < NUM_AXIS; i++)
    {
        Serial.print("Axis ");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(currentpos[i]);
    }
}

/* #####################################################################################################







Rotation encoder check and response
*/

void checkEncoder()
{
#define PATTERN_PRESS 0b00101111
    static byte rota_debouncer = 0;
    static byte rota_recognized = false;

    rota_debouncer = (rota_debouncer << 1) | (digitalRead(PIN_ROTA_1) == LOW ? 1 : 0);
    if ((rota_debouncer & PATTERN_PRESS) == PATTERN_PRESS)
    {
        if (rota_recognized == false)
        {
            rota_recognized = true;
            if (digitalRead(PIN_ROTA_2))
                turnDown();
            else
                turnUp();
        }
    }
    else
    {
        rota_recognized = false;
    }
}

/* #####################################################################################################






React to "knob being turned right/up"
*/

void turnUp()
{
    switch (panelMode)
    {
    case PM_MAINMENU:
        mainmenu_nextoption();
        break;

    case PM_DELAYSET:
        smartDelayStep();
        stepDelay = stepDelay + stepDelayStep;
        recalculateDelay();
        showPositionAndTarget();
        break;

    case PM_MOPASET: // selection only; will active only on encoder button press
        motionPatternSelect = (motionPatternSelect == countof(motionPatternTexts) - 1 ? 0 : motionPatternSelect + 1);
        break;

    case PM_STARTSET:
    case PM_ENDSET:
        // turn instead of press returns to mainmenu_option
        panelMode = PM_MAINMENU;
        break;

    default:
        break;
    }

    disp_panelmode();
}
/* #####################################################################################################







React to "knob being turned left/down"
*/
void turnDown()
{
    switch (panelMode)
    {
    case PM_MAINMENU:
        mainmenu_previousoption();
        break;

    case PM_DELAYSET:
        smartDelayStep();
        stepDelay = stepDelay - (stepDelay >= stepDelayStep ? stepDelayStep : 0);
        recalculateDelay();
        showPositionAndTarget();
        break;

    case PM_MOPASET:
        motionPatternSelect = (motionPatternSelect == 0 ? countof(motionPatternTexts) - 1 : motionPatternSelect - 1);
        break;

    case PM_STARTSET:
    case PM_ENDSET:
        panelMode = PM_MAINMENU;
        break;

    default:
        break;
    }

    disp_panelmode();
}

/* #####################################################################################################



Copy the current position into the storage slot "cursor"
*/
void currentPosToStorage(byte cursor)
{
    for (byte i = 0; i < NUM_AXIS; i++)
    {
        positions[cursor][i] = currentpos[i];
    }

    posCursor = cursor;
    setTargetToPositionItem(cursor);
}

/* #####################################################################################################




Show the output for the current panel mode in the lower line
*/
void disp_panelmode()
{

    // top line may be occupied by info display so the
    // lower line shows either option selector or option's items

    switch (panelMode)
    {
    case PM_MAINMENU:
        snprintf(strbuf, sizeof strbuf, "%S", mainmenu_texts[mainmenu_option]);
        break;

    case PM_DELAYSET:
        snprintf(strbuf, sizeof strbuf, "Step Delay %05u", stepDelay);
        break;
    case PM_STARTSET:
        // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
        snprintf(strbuf, sizeof strbuf, "%S", romTexts[0]);
        break;
    case PM_ENDSET:
        // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
        snprintf(strbuf, sizeof strbuf, "%S", romTexts[1]);
        break;
    case PM_MOPASET:
        // ignore the wchar_t / %S warning, arduino uses %S for Progmem instead
        snprintf(strbuf, sizeof strbuf, "Mode %.11S", motionPatternTexts[motionPatternSelect]);
        break;
    case PM_CONFIRMPOS:
        snprintf(strbuf, sizeof strbuf, "%S", romTexts[8]);
        panelMode = PM_MAINMENU;
        break;
    case PM_CONFIRMMODE:
        snprintf(strbuf, sizeof strbuf, "%S", romTexts[9]);
        panelMode = PM_MAINMENU;
        break;
    default:
        snprintf(strbuf, sizeof strbuf, "%c%S", 3, romTexts[7]);
    }
    lcd.print(1, 0, strbuf);
}
/* #####################################################################################################







Button debouncer logic and button mode detection
 */
void multiDebouncer()
{
#define dbPATTERN_PRESS 0b01001111
#define dbPATTERN_RELEASE 0b00011111
    static byte dbPins[] = {PUSH_ENCODER, PUSH_SKIP, PUSH_STASTOP};
    static byte b_flow[] = {0, 0, 0, 0, 0};
    static unsigned long lastTick = 0;
    static unsigned long nextTick = 0;
    static unsigned long buttonTicks[] = {0, 0, 0, 0, 0};

    for (byte i = 0; i < sizeof dbPins; i++)
    {
        buttonState[i] = BUTTON_PRESS_NONE; // always clear the state
    }

    if (theTick < nextTick)
    {
        return; // do a run at most every 5 ms
    }

    nextTick = theTick + 5;

    for (byte i = 0; i < sizeof dbPins; i++)
    {

        b_flow[i] = b_flow[i] << 1 | (digitalRead(dbPins[i]) == LOW ? 1 : 0);

        if ((b_flow[i] & dbPATTERN_PRESS) == dbPATTERN_PRESS)
        {
            buttonTicks[i] = buttonTicks[i] + (theTick - lastTick);
        }

        if ((b_flow[i] & dbPATTERN_RELEASE) == 0)
        {
            if (buttonTicks[i] > 0)
            {

                if (buttonTicks[i] < BUTTON_TIME_LONG)
                {
                    buttonState[i] = BUTTON_PRESS_SHORT;
                }
                else
                {
                    buttonState[i] = BUTTON_PRESS_LONG;
                }
            }
            buttonTicks[i] = 0;
        }
    }
    lastTick = theTick;
};

/*

Calculate deltas and step direction for Bresenham's algorithm using the N-dimensional position arrays currentpos / targetpos
Set direction pins too, since direction does not change until target reached (or break/stop)

*/
void recalculateMotion()
{
    stepsmax = 0;

    for (int i = 0; i < NUM_AXIS; i++)
    {
        steps[i] = targetpos[i] - currentpos[i];
        if (steps[i] > 0)
        {
            steps_dir[i] = 1;
            digitalWrite(motor_pins[i].pin_dir, motor_pins[i].lvl_forw);
        }
        else
        {
            steps_dir[i] = -1;
            steps[i] = -steps[i];
            digitalWrite(motor_pins[i].pin_dir, motor_pins[i].lvl_back);
        }

        if (steps[i] > stepsmax)
        {
            stepsmax = steps[i];
        }
    }

    stepstodo = stepsmax;
    for (int i = 0; i < NUM_AXIS; i++)
    {
        bresenham[i] = stepsmax / 2;
    }
}

void recalculateDelay()
{
    switch (mainMode)
    {
    case MODE_PREVIEW:
        tickerLimit = 0;
        break;
    default:
        switch (motionPatternActive)
        {
        case MOPA_ONEWAY:
        case MOPA_ONEWAY_REPEAT:
            if (posCursor == 0)
            {
                tickerLimit = 0;
            }
            else
            {
                tickerLimit = stepDelay;
            }
            break;
        default:
            tickerLimit = stepDelay;
            break;
        }
    }
}
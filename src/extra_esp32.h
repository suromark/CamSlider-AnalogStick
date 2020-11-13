#include "Arduino.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "BluetoothSerial.h"

bool setupExtra();
void loopExtra();
void parseForCommand(char *payload);

BluetoothSerial SerialBT;

bool setupExtra()
{
    char devicename[32];

    if (!btStart())
    {
        Serial.println("Failed to initialize controller");
        return false;
    }

    if (esp_bluedroid_init() != ESP_OK)
    {
        Serial.println("Failed to initialize bluedroid");
        return false;
    }

    if (esp_bluedroid_enable() != ESP_OK)
    {
        Serial.println("Failed to enable bluedroid");
        return false;
    }

    const uint8_t *point = esp_bt_dev_get_address();

    snprintf(devicename, sizeof devicename, "CamSlider_%02X%02X%02X%02X%02X%02X",
             point[0], point[1], point[2], point[3], point[4], point[5]);

    SerialBT.begin(devicename); // device name

    snprintf(strbuf, sizeof strbuf, "%S", romTexts[13]);
    lcd.print(0, 0, strbuf);
    return true;
}

void loopExtra()
{
    static char oneChar;
    static char inbuffer[256];
    static int insertpoint = 0;
    if (Serial.available())
    {
        oneChar = Serial.read();
        inbuffer[insertpoint] = oneChar;
        SerialBT.write(oneChar);
        insertpoint++;
        if (insertpoint >= sizeof inbuffer)
        {
            insertpoint = 0;
        }
        inbuffer[insertpoint] = 0;
        if (oneChar == '\n')
        {
            insertpoint = 0;
            parseForCommand(inbuffer);
            SerialBT.println("RECV");
        }
    }
    if (SerialBT.available())
    {
        oneChar = SerialBT.read();
        inbuffer[insertpoint] = oneChar;
        Serial.write(oneChar);
        insertpoint++;
        if (insertpoint >= sizeof inbuffer)
        {
            insertpoint = 0;
        }
        inbuffer[insertpoint] = 0;
        if (oneChar == '\n')
        {
            insertpoint = 0;
            SerialBT.println("RCVD");
            parseForCommand(inbuffer);
        }
    }
}

void parseForCommand(char *payload)
{
    static char echobuffer[64];
    static char pattern[32];
    int hits = 0;
    int understood = 0;
    long theLong = 0;
    int theInt = 0; // storage for an int parameter

    /* check for commands - format: 
    target absolute: a<AXIS>,<VALUE> e.g. a0,-123 
    target relative: r<AXIS>,<VALUE> e.g. r0,-123 
    target store: s<AXIS>,<VALUE>,<SLOT> e.g. s2,5000,0 
    step delay: sd<VALUE>
    run to current target: g
    run to stored target <N>: t<N>
    */
    for (byte i = 0; i < NUM_AXIS; i++)
    {
        snprintf(pattern, sizeof pattern, "a%d,%%d", i);
        hits = sscanf(payload, pattern, &theLong);
        if (hits == 1)
        {
            targetpos[i] = theLong;
            SerialBT.println(F("ABS POS"));
            understood = 1;
        }
        snprintf(pattern, sizeof pattern, "r%d,%%d", i);
        hits = sscanf(payload, pattern, &theLong);
        if (hits == 1)
        {
            targetpos[i] = targetpos[i] + theLong;
            SerialBT.println(F("REL POS"));
            understood = 1;
        }
        snprintf(pattern, sizeof pattern, "s%d,%%d,%%u", i);
        hits = sscanf(payload, pattern, &theLong, &theInt);
        if (hits == 2 && theInt >= 0 && theInt < numOfPoints)
        {
            positions[theInt][i] = theLong;
            SerialBT.println(F("SET STORAGE"));
            understood = 1;
        }
    }

    if (strcmp("zero\n", payload) == 0)
    {
        understood = 1;
        for (byte i = 0; i < NUM_AXIS; i++)
        {
            currentpos[i] = 0;
        }
        SerialBT.println(F("CUR.POS NOW ZERO"));
    }

    if (strcmp("g\n", payload) == 0)
    {
        understood = 1;
        SerialBT.println(F("START"));
        recalculateDelay();
        recalculateMotion();
        startFromHalt();
    }

    hits = sscanf(payload, "sd%u", &theLong); // set with Delay value
    if (hits == 1)
    {
        understood = 1;
        stepDelay = (unsigned int)theLong;
        SerialBT.println(F("SET STEP DELAY"));
        recalculateDelay();
        recalculateMotion();
    }

    hits = sscanf(payload, "t%u", &theInt); // go to stored target <N>
    if (hits == 1 && theInt >= 0 && theInt < numOfPoints)
    {
        understood = 1;
        SerialBT.println(F("GOTO PRESET"));
        if (setTargetToPositionItem(theInt))
        {
            recalculateDelay();
            recalculateMotion();
            startFromHalt();
        }
    }

    /* diagnostic echo */

    if (understood)
    {
        SerialBT.println("+OK");
        snprintf(echobuffer, sizeof echobuffer, "%S", romTexts[14] );
        lcd.print(0,0,echobuffer);
    }
    else
    {
        SerialBT.println("-ERR");
    }

    for (byte i = 0; i < NUM_AXIS; i++)
    {
        snprintf(echobuffer, sizeof echobuffer, "a%x,%d,%d", i, targetpos[i], currentpos[i]);
        SerialBT.println(echobuffer);
    }
}
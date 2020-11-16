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
    static unsigned long btKeepalive = 0;

    /*
    if( millis() - btKeepalive > 1000 ) {
        SerialBT.println("READY");
        btKeepalive = millis();
    }
*/

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
    byte understood = 0;
    long theLong = 0;
    int theInt = 0; // storage for an int parameter
    byte run_now = 0;

    /* check for commands - see readme.md for syntax */
    for (byte i = 0; i < NUM_AXIS; i++)
    {
        snprintf(pattern, sizeof pattern, "n%d,%%d", i);
        hits = sscanf(payload, pattern, &theLong);
        if (hits == 1)
        {
            for (byte j = 0; j < NUM_AXIS; j++)
            {
                targetpos[j] = currentpos[j];
                if (j == i)
                {
                    targetpos[j] += theLong;
                }
            }
            SerialBT.println(F("NUDGE_GO"));
            understood = 'N';
            run_now = 1;
            break;
        }

        snprintf(pattern, sizeof pattern, "a%d,%%d", i);
        hits = sscanf(payload, pattern, &theLong);
        if (hits == 1)
        {
            targetpos[i] = theLong;
            SerialBT.println(F("ABS POS"));
            understood = 'A';
            break;
        }
        snprintf(pattern, sizeof pattern, "r%d,%%d", i);
        hits = sscanf(payload, pattern, &theLong);
        if (hits == 1)
        {
            targetpos[i] = targetpos[i] + theLong;
            SerialBT.println(F("REL POS"));
            understood = 'R';
            break;
        }
        snprintf(pattern, sizeof pattern, "s%d,%%d,%%u", i);
        hits = sscanf(payload, pattern, &theLong, &theInt);
        if (hits == 2 && theInt >= 0 && theInt < numOfPoints)
        {
            positions[theInt][i] = theLong;
            SerialBT.println(F("SET STORAGE"));
            understood = 'S';
            break;
        }
    }

    if (strcmp("zero\n", payload) == 0)
    {
        understood = 'Z';
        for (byte i = 0; i < NUM_AXIS; i++)
        {
            currentpos[i] = 0;
        }
        SerialBT.println(F("CUR.POS NOW ZERO"));
    }

    if (strcmp("pos\n", payload) == 0)
    {
        understood = 'P';
        for (byte i = 0; i < NUM_AXIS; i++)
        {
            snprintf(echobuffer, sizeof echobuffer, "a%x,%d,%d", i, targetpos[i], currentpos[i]);
            SerialBT.println(echobuffer);
        }
    }

    if (strcmp("g\n", payload) == 0 || run_now)
    {
        understood = 'G';
        SerialBT.println(F("START"));
        recalculateDelay();
        recalculateMotion();
        startFromHalt();
    }

    hits = sscanf(payload, "sd%u", &theLong); // set with Delay value
    if (hits == 1)
    {
        understood = 'D';
        stepDelay = (unsigned int)theLong;
        SerialBT.println(F("SET STEP DELAY"));
        recalculateDelay();
        recalculateMotion();
    }

    hits = sscanf(payload, "set%u", &theInt); // save target into Slot <N>
    if (hits == 1 && theInt >= 0 && theInt < numOfPoints)
    {
        understood = 's';
        SerialBT.println(F("SAVE TO PRESET"));
        currentPosToStorage(theInt);
    }

    hits = sscanf(payload, "t%u", &theInt); // go to stored target <N>
    if (hits == 1 && theInt >= 0 && theInt < numOfPoints)
    {
        understood = 'T';
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
        snprintf(echobuffer, sizeof echobuffer, "%S", romTexts[14]);
        echobuffer[15] = understood;
        lcd.print(0, 0, echobuffer);
    }
    else
    {
        SerialBT.println("-ERR");
    }
}
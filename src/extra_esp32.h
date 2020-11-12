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
            parseForCommand(inbuffer);
            SerialBT.println("RECV");
        }
    }
}

void parseForCommand(char *payload)
{
    int hits = 0;
    long theValue = 0;
    hits = sscanf(payload, "x%d", &theValue); // Set X target
    if (hits == 1)
    {
        targetpos[0] = theValue;
    }
    hits = sscanf(payload, "y%d", &theValue); // Set Y target
    if (hits == 1)
    {
        targetpos[1] = theValue;
    }
    hits = sscanf(payload, "z%d", &theValue); // Set Z target
    if (hits == 1)
    {
        targetpos[2] = theValue;
    }
    hits = sscanf(payload, "g%u", &theValue); // Go with Delay value
    if (hits == 1)
    {
        SerialBT.println("EXEC");
        stepDelay = theValue;
        recalculateDelay();
        recalculateMotion();
        startFromHalt();
    }
    if (strcmp("zero", payload) == 0)
    {
        for (byte i = 0; i < NUM_AXIS; i++)
        {
            currentpos[i] = 0;
        }
        SerialBT.println("ZERO");
    }
}
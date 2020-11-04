#include "Arduino.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"
#include "myLCD.h"

myLCD::myLCD()
{
    _adrlcd = 0; // Default LCD module base address on I2C bus; 
}
/*



   Check if there is a device at I2C <addr> 
*/
bool myLCD::check_i2c(uint8_t adr)
{
    byte error;
    Wire.begin();
    Wire.beginTransmission(adr);
    error = Wire.endTransmission();

    if (error == 0)
    {
        Serial.print(F("I2C device found at address 0x"));
        if (adr < 16)
            Serial.print("0");
        Serial.print(adr, HEX);
        Serial.println("  !");
        myLCD::_adrlcd = adr;
        return true;
    }
    else
    {
        Serial.print(F("Error 0x"));
        Serial.println(error, HEX);
        Serial.print(F(" at address 0x"));
        if (adr < 16)
            Serial.print("0");
        Serial.println(adr, HEX);
        return false;
    }
}

void myLCD::setup(uint8_t adr)
{

    _lcd = new LiquidCrystal_I2C(adr, 16, 2); // if no LCD detected, this will never be called

    uint8_t myarrow[8] = {
        0b00000,
        0b11000,
        0b11100,
        0b11110,
        0b11100,
        0b11000,
        0b00000,
        0b00000};

    if (!check_i2c(adr))
        _adrlcd = 0;

    if (_adrlcd)
    {
        _lcd->init();
        _lcd->backlight();
        _lcd->home();
        _lcd->createChar(3, myarrow);
    }
}

void myLCD::clear()
{
    if (!_adrlcd)
        return;

    _lcd->clear();
}

void myLCD::lighton()
{
    if (!_adrlcd)
        return;

    _lcd->backlight();
}

void myLCD::lightoff()
{
    if (!_adrlcd)
        return;

    _lcd->noBacklight();
}

void myLCD::print(uint8_t row, uint8_t col, char *ctext)
{
    if (_adrlcd == 0)
        return;
    // Serial.println( ctext );
    _lcd->setCursor(col, row);
    _lcd->printstr(ctext);
    // lcd.print(*ctext);
}

void myLCD::cursormark(uint8_t row, uint8_t col)
{
    return; // not useful
    if (_adrlcd == 0)
        return;
    _lcd->setCursor(col, row);
    _lcd->cursor();
}
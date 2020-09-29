#include "Wire.h"
#include "LiquidCrystal_I2C.h"

uint8_t adr_lcd = 0x27; // Default LCD module base address on I2C bus

LiquidCrystal_I2C lcd(0x27, 16, 2); // if no LCD detected, this will never be called

/*



   Check if there is a device at I2C <addr> 
*/
bool check_i2c(uint8_t adr)
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

void lcdsetup()
{

    uint8_t myarrow[8] = {
        0b11000,
        0b11100,
        0b11110,
        0b11111,
        0b11110,
        0b11100,
        0b11000,
        0b00000
    };

    if (!check_i2c(adr_lcd))
        adr_lcd = 0;

    if (adr_lcd)
    {
        lcd.init();
        lcd.backlight();
        lcd.home();
        lcd.createChar(3, myarrow);
    }
}

void lcdclear()
{
    if (!adr_lcd)
        return;

    lcd.clear();
}

void lcdprint(uint8_t row, uint8_t col, char *ctext)
{
    if (adr_lcd == 0)
        return;
    // Serial.println( ctext );
    lcd.setCursor(col, row);
    lcd.printstr(ctext);
    // lcd.print(*ctext);
}

void lcdcursormark( uint8_t row, uint8_t col )
{
    return; // not useful
    if( adr_lcd == 0 ) return;
    lcd.setCursor( col, row);
    lcd.cursor();
}
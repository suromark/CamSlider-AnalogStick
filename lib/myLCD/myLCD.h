#ifndef myLCD_h
#define myLCD_h

#include "Arduino.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

class myLCD {
    private:
    uint8_t _adrlcd = 0x27; // usual i2c address of 1604 display
    bool check_i2c(uint8_t adr);

    public:
    myLCD();
    LiquidCrystal_I2C* _lcd;
    void setup();
    void lighton();
    void lightoff();
    void clear();
    void print(uint8_t row, uint8_t col, char *ctext);
    void cursormark( uint8_t row, uint8_t col );
};

#endif
#ifndef myLCD_h
#define myLCD_h

#include "Arduino.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

class myLCD {
    private:
    uint8_t _adrlcd = 0; // see globals.h for actual value
    bool check_i2c(uint8_t adr);

    public:
    myLCD();
    LiquidCrystal_I2C* _lcd;
    bool setup( uint8_t adr ); // setup i2c LCD at I2C adress adr
    void lighton();
    void lightoff();
    void clear();
    void print(uint8_t row, uint8_t col, char *ctext);
    void cursormark( uint8_t row, uint8_t col );
};

#endif
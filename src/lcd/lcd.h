#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include "../config.h"
#include <LiquidCrystal_I2C.h>

class LCDDisplay {
private:
    LiquidCrystal_I2C lcd;
    
public:
    LCDDisplay();
    
    void init();
    void printLine(uint8_t line, const String& text);
    void printCentered(uint8_t line, const String& text);
    void clear();
    void backlight(bool on);
    bool testConnection();
    void test();
};

#endif
#include "lcd.h"

LCDDisplay::LCDDisplay() : lcd(LCD_I2C_ADDR, 16, 2) {
}

void LCDDisplay::init() {
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    
    debugLog("LCD Display initialized at I2C address 0x%02X", LCD_I2C_ADDR);
}

void LCDDisplay::printLine(uint8_t line, const String& text) {
    if (line > 1) return;
    
    lcd.setCursor(0, line);
    
    String padded = text;
    while (padded.length() < 16) {
        padded += " ";
    }
    
    lcd.print(padded.substring(0, 16));
}

void LCDDisplay::printCentered(uint8_t line, const String& text) {
    if (line > 1) return;
    
    int textLen = text.length();
    int startPos = (16 - textLen) / 2;
    if (startPos < 0) startPos = 0;
    
    String padded = "";
    for (int i = 0; i < startPos; i++) padded += " ";
    padded += text;
    while (padded.length() < 16) padded += " ";
    
    lcd.setCursor(0, line);
    lcd.print(padded.substring(0, 16));
}

void LCDDisplay::clear() {
    lcd.clear();
}

void LCDDisplay::backlight(bool on) {
    if (on) {
        lcd.backlight();
    } else {
        lcd.noBacklight();
    }
}

bool LCDDisplay::testConnection() {
    lcd.setCursor(0, 0);
    lcd.print("LCD OK");
    delay(500);
    lcd.clear();
    return true;
}

void LCDDisplay::test() {
    Serial.println("\n╔─ TEST LCD ────────────────────────╗");
    printCentered(0, "Test LCD 16x2");
    printCentered(1, "OK!");
    delay(2000);
    clear();
    Serial.println("║ Test LCD complet ✓                ║");
    Serial.println("╚─────────────────────────────────────╝\n");
}
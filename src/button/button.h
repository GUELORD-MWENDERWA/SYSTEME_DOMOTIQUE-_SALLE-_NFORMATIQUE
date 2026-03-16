#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include "../config.h"

class ButtonManager {
private:
    struct ButtonState {
        uint8_t pin;
        bool currentState;
        bool previousState;
        uint32_t pressTime;
        bool longPressDetected;
    };
    
    ButtonState buttons[2];
    
    void debounce(ButtonState& btn);
    
public:
    ButtonManager();
    
    void init();
    void update();
    bool wasPressed(uint8_t pin);
    bool wasLongPressed(uint8_t pin);
    bool isPressed(uint8_t pin);
    void test();
};

#endif
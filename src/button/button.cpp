#include "button.h"

ButtonManager::ButtonManager() {
    buttons[0] = {BUTTON_MODE_PIN, HIGH, HIGH, 0, false};
    buttons[1] = {BUTTON_LAMP_PIN, HIGH, HIGH, 0, false};
}

void ButtonManager::init() {
    pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);
    pinMode(BUTTON_LAMP_PIN, INPUT_PULLUP);
    
    debugLog("Button manager initialized on pins: MODE=%d LAMP=%d", 
             BUTTON_MODE_PIN, BUTTON_LAMP_PIN);
}

void ButtonManager::debounce(ButtonState& btn) {
    bool reading = digitalRead(btn.pin);
    
    if (reading != btn.previousState) {
        delay(BUTTON_DEBOUNCE_MS);
        reading = digitalRead(btn.pin);
    }
    
    btn.previousState = btn.currentState;
    btn.currentState = reading;
}

void ButtonManager::update() {
    for (int i = 0; i < 2; i++) {
        debounce(buttons[i]);
        
        if (buttons[i].currentState == LOW && buttons[i].previousState == HIGH) {
            buttons[i].pressTime = millis();
            buttons[i].longPressDetected = false;
        }
        
        if (buttons[i].currentState == LOW && 
            millis() - buttons[i].pressTime > BUTTON_LONG_PRESS_MS &&
            !buttons[i].longPressDetected) {
            buttons[i].longPressDetected = true;
        }
    }
}

bool ButtonManager::wasPressed(uint8_t pin) {
    for (int i = 0; i < 2; i++) {
        if (buttons[i].pin == pin) {
            if (buttons[i].currentState == HIGH && buttons[i].previousState == LOW) {
                if (!buttons[i].longPressDetected) {
                    return true;
                }
                buttons[i].longPressDetected = false;
                return false;
            }
        }
    }
    return false;
}

bool ButtonManager::wasLongPressed(uint8_t pin) {
    for (int i = 0; i < 2; i++) {
        if (buttons[i].pin == pin) {
            if (buttons[i].longPressDetected) {
                if (buttons[i].currentState == HIGH) {
                    buttons[i].longPressDetected = false;
                    return true;
                }
            }
        }
    }
    return false;
}

bool ButtonManager::isPressed(uint8_t pin) {
    for (int i = 0; i < 2; i++) {
        if (buttons[i].pin == pin) {
            return buttons[i].currentState == LOW;
        }
    }
    return false;
}

void ButtonManager::test() {
    debugLog("Button test - press each button");
    delay(3000);
}
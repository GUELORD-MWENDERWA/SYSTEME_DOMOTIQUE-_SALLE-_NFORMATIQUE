#include "relay.h"

RelayController::RelayController() : registerState(0x00) {
}

void RelayController::init() {
    pinMode(HC595_LATCH_PIN, OUTPUT);
    pinMode(HC595_CLOCK_PIN, OUTPUT);
    pinMode(HC595_DATA_PIN, OUTPUT);
    
    digitalWrite(HC595_LATCH_PIN, LOW);
    digitalWrite(HC595_CLOCK_PIN, LOW);
    digitalWrite(HC595_DATA_PIN, LOW);
    
    setAllOff();
    debugLog("74HC595 initialized on LATCH=%d CLOCK=%d DATA=%d", 
             HC595_LATCH_PIN, HC595_CLOCK_PIN, HC595_DATA_PIN);
}

void RelayController::updateRegister() {
    // Ensure latch is LOW
    digitalWrite(HC595_LATCH_PIN, LOW);
    delayMicroseconds(20);

    // Shift out 8 bits (LSB first)
    for (int i = 0; i < 8; i++) {
        digitalWrite(HC595_DATA_PIN, (registerState >> i) & 1);
        delayMicroseconds(10);
        digitalWrite(HC595_CLOCK_PIN, HIGH);
        delayMicroseconds(20);
        digitalWrite(HC595_CLOCK_PIN, LOW);
        delayMicroseconds(10);
    }

    // Latch the data
    delayMicroseconds(20);
    digitalWrite(HC595_LATCH_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(HC595_LATCH_PIN, LOW);
    delayMicroseconds(10);
}

void RelayController::setOutput(uint8_t channel, bool state) {
    if (channel > 7) return;

    if (state) {
        registerState |= (1 << channel);
    } else {
        registerState &= ~(1 << channel);
    }

    updateRegister();
}

void RelayController::setAllOn() {
    registerState = 0xFF;
    updateRegister();
}

void RelayController::setAllOff() {
    registerState = 0x00;
    updateRegister();
}

bool RelayController::getOutputState(uint8_t channel) const {
    if (channel > 7) return false;
    return (registerState >> channel) & 1;
}

uint8_t RelayController::getMask() const {
    return registerState;
}

void RelayController::setBulk(uint8_t mask) {
    registerState = mask;
    updateRegister();
}

void RelayController::test() {
    debugLog("Testing 74HC595 outputs...");
    
    for (int ch = 0; ch < 8; ch++) {
        setOutput(ch, true);
        delay(200);
        setOutput(ch, false);
    }
    
    debugLog("74HC595 test complete");
}
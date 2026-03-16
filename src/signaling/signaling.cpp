#include "signaling.h"

SignalManager::SignalManager(RelayController* relay) : relayCtrl(relay) {
}

void SignalManager::init() {
    if (relayCtrl == nullptr) {
        debugLog("ERROR: SignalManager not properly initialized");
        return;
    }
    debugLog("Signal Manager initialized");
}

void SignalManager::setLEDGreen(bool state) {
    if (relayCtrl != nullptr) {
        relayCtrl->setOutput(Q7_LED_GREEN, state);
    }
}

void SignalManager::setLEDRed(bool state) {
    if (relayCtrl != nullptr) {
        relayCtrl->setOutput(Q6_LED_RED, state);
    }
}

void SignalManager::setBuzzer(bool state) {
    if (relayCtrl != nullptr) {
        relayCtrl->setOutput(Q5_BUZZER, state);
    }
}

void SignalManager::shortBeep() {
    beep(100, 50);
}

void SignalManager::longBeep() {
    beep(300, 100);
}

void SignalManager::beep(uint16_t durationMs, uint16_t delayMs) {
    setBuzzer(true);
    delay(durationMs);
    setBuzzer(false);
    delay(delayMs);
}

void SignalManager::ledOnForMs(uint8_t ledPin, uint16_t durationMs) {
    // Simplified: use setLED with pin (not implemented detail)
    // For now, this is handled via relayCtrl->setOutput directly
}

void SignalManager::ledPulse(uint8_t ledPin, uint16_t cycleMs, uint8_t pulses) {
    uint16_t halfCycle = cycleMs / 2;
    
    for (uint8_t i = 0; i < pulses; i++) {
        if (ledPin == Q7_LED_GREEN) setLEDGreen(true);
        else if (ledPin == Q6_LED_RED) setLEDRed(true);
        
        delay(halfCycle);
        
        if (ledPin == Q7_LED_GREEN) setLEDGreen(false);
        else if (ledPin == Q6_LED_RED) setLEDRed(false);
        
        delay(halfCycle);
    }
}

const char* SignalManager::getEventName(EventType event) {
    switch (event) {
        case EVENT_SYSTEM_STARTUP: return "SYSTEM_STARTUP";
        case EVENT_MODE_REGISTRATION: return "MODE_REGISTRATION";
        case EVENT_MODE_ACCESS: return "MODE_ACCESS";
        case EVENT_ACCESS_GRANTED: return "ACCESS_GRANTED";
        case EVENT_ACCESS_DENIED: return "ACCESS_DENIED";
        case EVENT_CARD_REGISTERED: return "CARD_REGISTERED";
        case EVENT_INTRUSION_ALERT: return "INTRUSION_ALERT";
        case EVENT_NIGHT_MODE: return "NIGHT_MODE";
        case EVENT_WARNING: return "WARNING";
        default: return "UNKNOWN";
    }
}

void SignalManager::signalEvent(EventType event) {
    debugLog("📢 Signal Event: %s", getEventName(event));
    
    switch (event) {
        case EVENT_SYSTEM_STARTUP:
            // 1 short beep + green flash
            setLEDGreen(true);
            shortBeep();
            delay(200);
            setLEDGreen(false);
            break;
            
        case EVENT_MODE_REGISTRATION:
            // 3 red flashes + 1 long beep
            for (int i = 0; i < 3; i++) {
                setLEDRed(true);
                delay(200);
                setLEDRed(false);
                delay(200);
            }
            longBeep();
            break;
            
        case EVENT_MODE_ACCESS:
            // 1 short beep + green flash
            setLEDGreen(true);
            shortBeep();
            delay(200);
            setLEDGreen(false);
            break;
            
        case EVENT_ACCESS_GRANTED:
            // 1 short beep + steady green for 500ms
            setLEDGreen(true);
            shortBeep();
            delay(400);
            setLEDGreen(false);
            break;
            
        case EVENT_ACCESS_DENIED:
            // 2 short beeps + steady red for 2 seconds
            setLEDRed(true);
            shortBeep();
            delay(100);
            shortBeep();
            delay(1700);
            setLEDRed(false);
            break;
            
        case EVENT_CARD_REGISTERED:
            // 1 short beep + quick green pulse
            setLEDGreen(true);
            shortBeep();
            delay(100);
            setLEDGreen(false);
            break;
            
        case EVENT_INTRUSION_ALERT:
            // 3 long beeps + red LED on
            setLEDRed(true);
            for (int i = 0; i < 3; i++) {
                beep(300, 200);
            }
            setLEDRed(false);
            break;
            
        case EVENT_NIGHT_MODE:
            // 1 short beep (minimal, just notification)
            shortBeep();
            break;
            
        case EVENT_WARNING:
            // 2 short beeps + orange effect (both LEDs)
            setLEDRed(true);
            setLEDGreen(true);
            shortBeep();
            delay(100);
            shortBeep();
            delay(500);
            setLEDRed(false);
            setLEDGreen(false);
            break;
            
        default:
            shortBeep();
            break;
    }
}

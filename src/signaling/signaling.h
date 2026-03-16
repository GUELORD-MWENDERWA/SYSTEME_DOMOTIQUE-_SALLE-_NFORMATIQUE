#ifndef SIGNALING_H
#define SIGNALING_H

#include <Arduino.h>
#include "../config.h"
#include "../relay/relay.h"

/**
 * Signal Manager
 * 
 * Unified management of LED (RED, GREEN) and Buzzer signaling
 * for all project events: mode changes, access control, intrusion alerts, etc.
 * 
 * Each event type has a specific visual and audio signature for user awareness.
 */
class SignalManager {
public:
    // Event types for signaling
    enum EventType {
        EVENT_SYSTEM_STARTUP = 0,      // 1 beep + green flash
        EVENT_MODE_REGISTRATION = 1,   // 3 red flashes + 1 long beep
        EVENT_MODE_ACCESS = 2,         // 1 short beep + green flash
        EVENT_ACCESS_GRANTED = 3,      // 1 short beep + steady green
        EVENT_ACCESS_DENIED = 4,       // 2 short beeps + steady red
        EVENT_CARD_REGISTERED = 5,     // 1 short beep + green flash
        EVENT_INTRUSION_ALERT = 6,     // 3 long beeps + pulsing red
        EVENT_NIGHT_MODE = 7,          // 1 beep (outside light on)
        EVENT_WARNING = 8              // 2 short beeps + orange (red+green)
    };

    SignalManager(RelayController* relay);

    /**
     * Initialize signaling system
     */
    void init();

    /**
     * Play an event signal (blocks briefly during signaling)
     */
    void signalEvent(EventType event);

    /**
     * Get human-readable event name
     */
    static const char* getEventName(EventType event);

private:
    RelayController* relayCtrl;

    // Helper functions for basic signals
    void beep(uint16_t durationMs, uint16_t delayMs = 100);
    void shortBeep();
    void longBeep();
    void ledOnForMs(uint8_t ledPin, uint16_t durationMs);
    void ledPulse(uint8_t ledPin, uint16_t cycleMs, uint8_t pulses);
    void setLEDGreen(bool state);
    void setLEDRed(bool state);
    void setBuzzer(bool state);
};

#endif

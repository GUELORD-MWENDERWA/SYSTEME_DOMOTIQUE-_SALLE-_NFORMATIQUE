#include "logic.h"

SystemLogic::SystemLogic() 
    : systemStatus(nullptr), relayCtrl(nullptr), servoCtrl(nullptr),
      rfidMgr(nullptr), energyMon(nullptr), ldrSensor(nullptr), storage(nullptr),
      lastIntrusionAlert(0), failed_access_counter(0), stateDirty(false) {
}

void SystemLogic::init(SystemStatus* status, RelayController* relay, ServoController* servo,
                       RFIDManager* rfid, EnergyMonitor* energy, LDRSensor* ldr,
                       StorageManager* storageManager) {
    systemStatus = status;
    relayCtrl = relay;
    servoCtrl = servo;
    rfidMgr = rfid;
    energyMon = energy;
    ldrSensor = ldr;
    storage = storageManager;

    debugLog("System Logic initialized");
}

void SystemLogic::handleRFIDScan(RFIDScanType scanType, uint32_t cardUID) {
    if (systemStatus == nullptr) return;
    
    systemStatus->state = STATE_CARD_SCANNED;
    systemStatus->state_transition_time = millis();
    
    systemStatus->last_rfid_scan = millis();
    
    if (systemStatus->current_mode == MODE_ACCESS) {
        AccessStatus status = rfidMgr->verifyAccess(cardUID);
        
        if (status == ACCESS_GRANTED) {
            CardData* card = rfidMgr->getCardByUID(cardUID);
            bool updatedCounters = false;

            if (card != nullptr) {
                if (scanType == RFID_ENTRY) {
                    if (!card->isInside) {
                        systemStatus->counters.entries_count++;
                        card->isInside = true;
                        updatedCounters = true;
                        // DISABLE intrusion alarm when someone enters
                        systemStatus->intrusion_detected = false;
                        debugLog("ENTRY: +1 | Total entries: %d | Intrusion alarm DISABLED", systemStatus->counters.entries_count);
                    } else {
                        debugLog("ENTRY ignored (card already inside): %08X", cardUID);
                    }
                } else {
                    if (card->isInside && systemStatus->counters.presence_count > 0) {
                        systemStatus->counters.exits_count++;
                        card->isInside = false;
                        updatedCounters = true;
                        debugLog("EXIT: +1 | Total exits: %d", systemStatus->counters.exits_count);
                    } else {
                        debugLog("EXIT ignored (card not inside): %08X", cardUID);
                    }
                }

                if (updatedCounters && storage != nullptr) {
                    storage->saveRegisteredCards(rfidMgr->getAuthorizedCards(), rfidMgr->getRegisteredCardsCount());
                }
            }

            if (updatedCounters) {
                systemStatus->counters.presence_count = 
                    max(0, (int)systemStatus->counters.entries_count - (int)systemStatus->counters.exits_count);
                debugLog("Presence count: %d", systemStatus->counters.presence_count);
            }

            executeAccessGrantedScenario();
        } else {
            executeAccessDeniedScenario();
        }
    } else if (systemStatus->current_mode == MODE_REGISTRATION) {
        rfidMgr->registerNewCard(cardUID);
        
        systemStatus->relays.q7_led_green = true;
        systemStatus->relays.q5_buzzer = true;
        updateRelayOutputs();
        delay(200);
        systemStatus->relays.q5_buzzer = false;
        systemStatus->relays.q7_led_green = false;
        updateRelayOutputs();
        
        debugLog("Card registered in REGISTRATION mode");
    }
    
    updateLightingLogic();
    stateDirty = true;
}

void SystemLogic::setSystemMode(SystemMode newMode) {
    if (systemStatus == nullptr) return;
    
    systemStatus->current_mode = newMode;
    
    if (newMode == MODE_REGISTRATION) {
        debugLog("System in REGISTRATION mode");
        
        // Clignotement LED rouge
        for (int i = 0; i < 3; i++) {
            systemStatus->relays.q6_led_red = true;
            updateRelayOutputs();
            delay(200);
            systemStatus->relays.q6_led_red = false;
            updateRelayOutputs();
            delay(200);
        }
        
        // Buzzeur 1 bip long
        systemStatus->relays.q5_buzzer = true;
        updateRelayOutputs();
        delay(300);
        systemStatus->relays.q5_buzzer = false;
        updateRelayOutputs();
        
    } else {
        debugLog("System in ACCESS mode");
        
        // LED verte
        systemStatus->relays.q7_led_green = true;
        updateRelayOutputs();
        delay(500);
        systemStatus->relays.q7_led_green = false;
        updateRelayOutputs();
        
        // Buzzeur 1 bip court
        systemStatus->relays.q5_buzzer = true;
        updateRelayOutputs();
        delay(100);
        systemStatus->relays.q5_buzzer = false;
        updateRelayOutputs();
    }

    stateDirty = true;
}

void SystemLogic::updateDayNightStatus(DayNightStatus status) {
    if (systemStatus == nullptr) return;
    
    systemStatus->daynight = status;
    debugLog("Day/Night status: %s", status == STATUS_DAY ? "DAY" : "NIGHT");
    
    updateLightingLogic();
    stateDirty = true;
}

void SystemLogic::updateRelayOutputs() {
    if (systemStatus == nullptr || relayCtrl == nullptr) return;
    
    uint8_t mask = 0;
    mask |= systemStatus->relays.q0_lamp_inside ? (1 << Q0_LAMP_INSIDE) : 0;
    mask |= systemStatus->relays.q1_lamp_outside ? (1 << Q1_LAMP_OUTSIDE) : 0;
    mask |= systemStatus->relays.q2_prise1 ? (1 << Q2_PRISE1) : 0;
    mask |= systemStatus->relays.q3_prise2 ? (1 << Q3_PRISE2) : 0;
    mask |= systemStatus->relays.q4_fan ? (1 << Q4_FAN) : 0;
    mask |= systemStatus->relays.q5_buzzer ? (1 << Q5_BUZZER) : 0;
    mask |= systemStatus->relays.q6_led_red ? (1 << Q6_LED_RED) : 0;
    mask |= systemStatus->relays.q7_led_green ? (1 << Q7_LED_GREEN) : 0;
    
    relayCtrl->setBulk(mask);
}

void SystemLogic::updateLightingLogic() {
    if (systemStatus == nullptr) return;

    // Update ONLY lighting relays
    systemStatus->relays.q0_lamp_inside = (systemStatus->counters.presence_count > 0);
    systemStatus->relays.q1_lamp_outside = (systemStatus->daynight == STATUS_NIGHT && 
                                           systemStatus->counters.presence_count == 0);
    
    updateRelayOutputs();
}

void SystemLogic::saveStateIfNeeded() {
    if (!stateDirty || storage == nullptr || systemStatus == nullptr) return;
    storage->saveSystemState(systemStatus);
    stateDirty = false;
}

void SystemLogic::updateSystemState() {
    if (systemStatus == nullptr) return;
    
    if (systemStatus->intrusion_detected) {
        // Play alarm only once every 5 seconds
        if (millis() - lastIntrusionAlert > 5000) {
            lastIntrusionAlert = millis();
            
            // 3 short beeps + red LED on
            for (int i = 0; i < 3; i++) {
                systemStatus->relays.q5_buzzer = true;
                updateRelayOutputs();
                delay(150);
                systemStatus->relays.q5_buzzer = false;
                updateRelayOutputs();
                delay(100);
            }
            
            systemStatus->relays.q6_led_red = true;
            updateRelayOutputs();
        }
    } else {
        systemStatus->relays.q6_led_red = false;
        updateRelayOutputs();
    }

    saveStateIfNeeded();
}

void SystemLogic::executeAccessGrantedScenario() {
    debugLog("✓ ACCESS GRANTED");
    systemStatus->state = STATE_ACCESS_GRANTED;
    systemStatus->state_transition_time = millis();
    
    systemStatus->counters.total_access_attempts++;
    failed_access_counter = 0;
    
    // Servo: 0° → 180°
    servoCtrl->open();
    debugLog("Servo opened (180°)");
    
    // Signalisation: LED verte + 1 bip court
    systemStatus->relays.q7_led_green = true;
    systemStatus->relays.q6_led_red = false;
    systemStatus->relays.q5_buzzer = true;
    updateRelayOutputs();
    delay(100);
    systemStatus->relays.q5_buzzer = false;
    updateRelayOutputs();
    
    debugLog("Access granted signaling: Green LED + 1 beep");
    
    delay(SERVO_OPEN_DURATION_MS);
    
    // Servo: 180° → 0°
    servoCtrl->close();
    debugLog("Servo closed (0°)");
    
    systemStatus->relays.q7_led_green = false;
    updateRelayOutputs();
    
    systemStatus->state = STATE_IDLE;
    stateDirty = true;
}

void SystemLogic::executeAccessDeniedScenario() {
    debugLog("✗ ACCESS DENIED");
    systemStatus->state = STATE_ACCESS_DENIED;
    systemStatus->state_transition_time = millis();
    
    systemStatus->counters.total_access_attempts++;
    systemStatus->counters.denied_access_count++;
    failed_access_counter++;
    
    // Signalisation: LED rouge + 2 bips courts
    systemStatus->relays.q6_led_red = true;
    systemStatus->relays.q7_led_green = false;
    systemStatus->relays.q5_buzzer = true;
    updateRelayOutputs();
    delay(100);
    systemStatus->relays.q5_buzzer = false;
    updateRelayOutputs();
    delay(100);
    systemStatus->relays.q5_buzzer = true;
    updateRelayOutputs();
    delay(100);
    systemStatus->relays.q5_buzzer = false;
    updateRelayOutputs();
    
    debugLog("Access denied signaling: Red LED + 2 beeps");
    
    delay(LED_ALERT_DURATION_MS);
    systemStatus->relays.q6_led_red = false;
    updateRelayOutputs();
    
    if (failed_access_counter >= 3) {
        debugLog("⚠ Multiple failed access attempts! (%d)", failed_access_counter);
    }
    
    systemStatus->state = STATE_IDLE;
}
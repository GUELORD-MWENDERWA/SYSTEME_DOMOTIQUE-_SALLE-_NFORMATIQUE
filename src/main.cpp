#include <Arduino.h>
#include "config.h"
#include "energy/energy.h"
#include "rfid/rfid.h"
#include "relay/relay.h"
#include "servo/servo.h"
#include "button/button.h"
#include "ldr/ldr.h"
#include "lcd/lcd.h"
#include "serial_cmd/serial_cmd.h"
#include "storage/storage.h"
#include "logic/logic.h"
#include "motion/motion.h"
#include "signaling/signaling.h"
#include "web/web_server.h"
#include <WiFi.h>

EnergyMonitor energyMonitor;
RFIDManager rfidManager;
RelayController relayController;
SignalManager signalManager(&relayController);
ServoController servoController;
ButtonManager buttonManager;
LDRSensor ldrSensor;
LCDDisplay lcdDisplay;
SerialCommandHandler serialHandler;
StorageManager storageManager;
SystemLogic systemLogic;
PIRMotionSensor motionSensor(MOTION_SENSOR_PIN);
WebServerManager webServer;

SystemStatus globalSystemStatus = {
    .current_mode = MODE_ACCESS,
    .state = STATE_IDLE,
    .daynight = STATUS_DAY,
    .counters = {0, 0, 0, 0, 0},
    .relays = {false, false, false, false, false, false, false, false},
    .intrusion_detected = false,
    .last_rfid_scan = 0,
    .state_transition_time = 0
};

uint32_t lastPZEMRead = 0;
uint32_t lastLCDRefresh = 0;
uint32_t lastLDRRead = 0;

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    delay(1000);
    
    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║  SYSTÈME DOMOTIQUE SALLE INFORMATIQUE  ║");
    Serial.println("║         Démarrage en cours...          ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    Serial.println("[INIT] Storage Manager...");
    storageManager.init();

    bool stateLoaded = storageManager.loadSystemState(&globalSystemStatus);
    if (!stateLoaded) {
        Serial.println("[INIT] No stored state found; using defaults.");
        storageManager.saveSystemState(&globalSystemStatus);
    } else {
        Serial.println("[INIT] Restored previous system state from EEPROM.");
    }

    delay(100);

    Serial.println("[INIT] Relay Controller (74HC595)...");
    relayController.init();

    // Restore relay outputs from stored state (if any)
    uint8_t relayMask = 0;
    relayMask |= globalSystemStatus.relays.q0_lamp_inside ? (1 << Q0_LAMP_INSIDE) : 0;
    relayMask |= globalSystemStatus.relays.q1_lamp_outside ? (1 << Q1_LAMP_OUTSIDE) : 0;
    relayMask |= globalSystemStatus.relays.q2_prise1 ? (1 << Q2_PRISE1) : 0;
    relayMask |= globalSystemStatus.relays.q3_prise2 ? (1 << Q3_PRISE2) : 0;
    relayMask |= globalSystemStatus.relays.q4_fan ? (1 << Q4_FAN) : 0;
    relayMask |= globalSystemStatus.relays.q5_buzzer ? (1 << Q5_BUZZER) : 0;
    relayMask |= globalSystemStatus.relays.q6_led_red ? (1 << Q6_LED_RED) : 0;
    relayMask |= globalSystemStatus.relays.q7_led_green ? (1 << Q7_LED_GREEN) : 0;
    relayController.setBulk(relayMask);

    delay(100);
    
    Serial.println("[INIT] Energy Monitor (PZEM-004T v3.0)...");
    energyMonitor.init();
    delay(200);
    
    Serial.println("[INIT] RFID Managers (RC522 x2)...");
    rfidManager.init(&storageManager);
    delay(200);
    
    Serial.println("[INIT] Servo Controller...");
    servoController.init();
    servoController.setAngle(0);
    delay(100);
    
    Serial.println("[INIT] LDR Sensor...");
    ldrSensor.init();
    delay(50);
    
    Serial.println("[INIT] LCD Display (I2C)...");
    lcdDisplay.init();
    lcdDisplay.printCentered(0, "Domotique Salle");
    lcdDisplay.printCentered(1, "Demarrage...");
    delay(200);
    
    Serial.println("[INIT] Button Manager...");
    buttonManager.init();
    delay(100);

    Serial.println("[INIT] Motion Sensor (PIR)...");
    motionSensor.init();
    delay(50);
    
    Serial.println("[INIT] Serial Command Handler...");
    serialHandler.init();
    
    Serial.println("[INIT] Signal Manager (LED + Buzzer)...");
    signalManager.init();
    signalManager.signalEvent(SignalManager::EVENT_SYSTEM_STARTUP);
    delay(100);
    
    Serial.println("[INIT] System Logic...");
    systemLogic.init(&globalSystemStatus, &relayController, &servoController,
                     &rfidManager, &energyMonitor, &ldrSensor, &storageManager);

    // Ensure outputs match the restored state
    systemLogic.updateDayNightStatus(globalSystemStatus.daynight);
    systemLogic.updateSystemState();

    // Initialize WiFi with smart mode selection
    Serial.println("[INIT] WiFi (STA mode with AP fallback)...");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setSleep(false);
    
    // SIMPLE AND RELIABLE: Use hardcoded defaults
    const char* wifiSSID = "702SH_AP";
    const char* wifiPass = "12345678___1";
    
    Serial.print("[WIFI] Connecting to: ");
    Serial.println(wifiSSID);
    
    WiFi.begin(wifiSSID, wifiPass);
    
    bool wifiConnected = false;
    uint32_t wifiStart = millis();
    
    // Try for 10 seconds
    while ((millis() - wifiStart) < 10000) {
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            break;
        }
        Serial.print(".");
        delay(500);
    }
    
    if (wifiConnected) {
        Serial.println("");
        Serial.print("[WIFI] ✓ Connected in ");
        Serial.print(millis() - wifiStart);
        Serial.print("ms | IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("");
        Serial.println("[WIFI] ✗ Connection timeout - starting AP mode");
        
        // Fallback to AP
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Domotique_Setup", "domotique2024");
        Serial.print("[WIFI] AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
    
    delay(100);

    Serial.println("[INIT] Web Server (Async)...");
    webServer.init(&globalSystemStatus, &systemLogic, &rfidManager, &relayController,
                   &energyMonitor, &storageManager);
    webServer.setAPMode(!wifiConnected); // Tell web server about AP mode
    webServer.begin();
    delay(100);

    Serial.println("\n[TEST] Lancement auto-test des composants...");
    delay(500);
    performSystemTest();
    
    Serial.println("\n╔══════���═════════════════════════════════╗");
    Serial.println("║      SYSTÈME PRÊT - Démarrage OK       ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    lcdDisplay.printCentered(0, "Systeme OK");
    lcdDisplay.printCentered(1, "Pret...");
    
    delay(1000);
}

void loop() {
    uint32_t currentTime = millis();
    
    if (Serial.available()) {
        serialHandler.processCommand();
    }
    
    if (currentTime - lastPZEMRead >= PZEM_READ_INTERVAL_MS) {
        lastPZEMRead = currentTime;
        energyMonitor.readValues();
    }
    
    static DayNightStatus stableDayNight = globalSystemStatus.daynight;
    static uint32_t lastDayNightChange = 0;
    static uint32_t lastMotionTrigger = 0;

    if (currentTime - lastLDRRead >= LDR_READ_INTERVAL_MS) {
        lastLDRRead = currentTime;
        uint16_t ldrValue = ldrSensor.readValue();

        // Inverted behavior: higher values = darker (night) for this LDR wiring
        DayNightStatus suggested = (ldrValue > LDR_DAY_THRESHOLD_HIGH) ? STATUS_NIGHT : 
                                   (ldrValue < LDR_DAY_THRESHOLD_LOW ? STATUS_DAY : stableDayNight);

        if (suggested != stableDayNight) {
            // First time we see a change, record the time
            if (lastDayNightChange == 0) {
                lastDayNightChange = millis();
            }
            // After stable time, apply the change
            if (millis() - lastDayNightChange > LDR_STABLE_MS) {
                stableDayNight = suggested;
                lastDayNightChange = 0;  // Reset for next change

                if (stableDayNight != globalSystemStatus.daynight) {
                    globalSystemStatus.daynight = stableDayNight;
                    systemLogic.updateDayNightStatus(stableDayNight);
                    debugLog("[LDR] Day/Night transition: %s (value=%d)", stableDayNight == STATUS_DAY ? "DAY" : "NIGHT", ldrValue);
                }
            }
        } else {
            // No change suggested, reset the timer
            lastDayNightChange = 0;
        }
    }

    // Motion detection (PIR) - only triggers intrusion alert when at night AND no presence
    motionSensor.update();
    
    if (globalSystemStatus.daynight == STATUS_NIGHT &&
        globalSystemStatus.counters.presence_count == 0 &&
        motionSensor.wasMotionDetected()) {
        
        if (millis() - lastMotionTrigger > MOTION_COOLDOWN_MS) {
            lastMotionTrigger = millis();
            debugLog("MOTION DETECTED AT NIGHT - INTRUSION ALERT");
            globalSystemStatus.intrusion_detected = true;
            storageManager.saveSystemState(&globalSystemStatus);
        }
    }
    
    buttonManager.update();
    
    if (buttonManager.wasLongPressed(BUTTON_MODE_PIN)) {
        SystemMode newMode = globalSystemStatus.current_mode == MODE_ACCESS ? 
                            MODE_REGISTRATION : MODE_ACCESS;
        systemLogic.setSystemMode(newMode);
    }
    
    if (buttonManager.wasPressed(BUTTON_LAMP_PIN)) {
        globalSystemStatus.relays.q0_lamp_inside = !globalSystemStatus.relays.q0_lamp_inside;
        systemLogic.updateRelayOutputs();
        storageManager.saveSystemState(&globalSystemStatus);
        debugLog("Lampe intérieure: %s (manuel)", globalSystemStatus.relays.q0_lamp_inside ? "ON" : "OFF");
    }
    
    if (rfidManager.scanReader(RFID_ENTRY)) {
        uint32_t cardUID = rfidManager.getLastScannedUID();
        systemLogic.handleRFIDScan(RFID_ENTRY, cardUID);
    }
    
    if (rfidManager.scanReader(RFID_EXIT)) {
        uint32_t cardUID = rfidManager.getLastScannedUID();
        systemLogic.handleRFIDScan(RFID_EXIT, cardUID);
    }
    
    if (currentTime - lastLCDRefresh >= LCD_REFRESH_MS) {
        lastLCDRefresh = currentTime;
        updateLCDDisplay();
    }
    
    systemLogic.updateSystemState();
    
    yield();
}

void updateLCDDisplay() {
    static uint8_t displayMode = 0;
    static uint32_t modeTimer = 0;
    
    if (millis() - modeTimer > 5000) {
        modeTimer = millis();
        displayMode = (displayMode + 1) % 4;
    }
    
    String line1 = "", line2 = "";
    
    switch (displayMode) {
        case 0: {
            // Live power/energy values
            line1 = "U:" + String(energyMonitor.getVoltage(), 1) + "V " +
                   "I:" + String(energyMonitor.getCurrent(), 2) + "A";
            line2 = "P:" + String(energyMonitor.getPower(), 1) + "W " +
                   "F:" + String(energyMonitor.getFrequency(), 1) + "Hz";
            break;
        }
        case 1: {
            // Presence and counters
            line1 = "Pres:" + String(globalSystemStatus.counters.presence_count) +
                   " Ent:" + String(globalSystemStatus.counters.entries_count);
            line2 = "Sort:" + String(globalSystemStatus.counters.exits_count) +
                   " T:" + getFormattedTime();
            break;
        }
        case 2: {
            // Mode and day/night
            line1 = globalSystemStatus.current_mode == MODE_ACCESS ? "Mode: ACCESS" : "Mode: REGISTR";
            line2 = globalSystemStatus.daynight == STATUS_DAY ? "JOUR" : "NUIT";
            break;
        }
        case 3: {
            // Relays / alarms
            line1 = "R0:" + String(relayController.getOutputState(Q0_LAMP_INSIDE) ? "1" : "0") +
                   " R1:" + String(relayController.getOutputState(Q1_LAMP_OUTSIDE) ? "1" : "0");
            line2 = "Alarm:" + String(globalSystemStatus.intrusion_detected ? "YES" : "NO");
            break;
        }
    }
    
    lcdDisplay.printCentered(0, line1);
    lcdDisplay.printCentered(1, line2);
}

void performSystemTest() {
    Serial.println("\n┌─ TEST COMPOSANTS ─────────────────────┐");
    
    Serial.print("├─ PZEM-004T v3.0... ");
    if (energyMonitor.testConnection()) {
        Serial.println("✓ OK");
    } else {
        Serial.println("⚠ Vérifier connexion");
    }
    
    Serial.print("├─ RFID Readers x2... ");
    if (rfidManager.testConnection()) {
        Serial.println("✓ OK");
    } else {
        Serial.println("⚠ Vérifier connexion SPI");
    }
    
    Serial.print("├─ 74HC595 (Relais)... ");
    relayController.setOutput(Q7_LED_GREEN, true);
    delay(100);
    relayController.setOutput(Q7_LED_GREEN, false);
    Serial.println("✓ OK");
    
    Serial.print("├─ Servo Motor... ");
    servoController.setAngle(90);
    delay(200);
    servoController.setAngle(0);
    Serial.println("✓ OK");
    
    Serial.print("├─ LDR Sensor... ");
    Serial.println("✓ OK (Value: " + String(ldrSensor.readValue()) + ")");
    
    Serial.print("├─ LCD I2C... ");
    Serial.println("✓ OK");
    
    Serial.print("├─ Buttons... ");
    Serial.println("✓ OK");
    
    Serial.println("└─ TEST COMPLET");
    Serial.println("────────────────────────────────────────\n");
}

void debugLog(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("[");
    Serial.print(getFormattedTime());
    Serial.print("] ");
    Serial.println(buffer);
}

String uint32ToHex(uint32_t value) {
    char buffer[9];
    sprintf(buffer, "%08X", value);
    return String(buffer);
}

String getFormattedTime() {
    uint32_t totalSeconds = millis() / 1000;
    uint8_t hours = (totalSeconds / 3600) % 24;
    uint8_t minutes = (totalSeconds / 60) % 60;
    uint8_t seconds = totalSeconds % 60;
    
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    return String(buffer);
}   
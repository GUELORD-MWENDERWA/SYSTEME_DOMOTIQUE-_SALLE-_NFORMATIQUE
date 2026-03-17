# ========== FICHIER COMPLET: src/relay/relay.cpp ==========

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

    registerState = 0x00;
    updateRegister();
    delay(50);
    debugLog("[RELAY] 74HC595 initialized");

}

void RelayController::updateRegister() {
// Ensure latch is LOW
digitalWrite(HC595_LATCH_PIN, LOW);
delayMicroseconds(10);

    // Shift out 8 bits
    for (int i = 0; i < 8; i++) {
        digitalWrite(HC595_DATA_PIN, (registerState >> i) & 1);
        delayMicroseconds(5);
        digitalWrite(HC595_CLOCK_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(HC595_CLOCK_PIN, LOW);
        delayMicroseconds(5);
    }

    // Latch the data
    delayMicroseconds(10);
    digitalWrite(HC595_LATCH_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(HC595_LATCH_PIN, LOW);

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
for (int ch = 0; ch < 8; ch++) {
setOutput(ch, true);
delay(150);
setOutput(ch, false);
delay(50);
}
debugLog("[RELAY] Test complete");
}

# ========== FICHIER COMPLET: src/rfid/rfid.cpp ==========

#include "rfid.h"

RFIDManager::RFIDManager()
: storage(nullptr), lastScannedUID(0), registeredCardsCount(0) {
ssPins[0] = RFID_SS2_PIN;
ssPins[1] = RFID_SS1_PIN;
}

void RFIDManager::init(StorageManager\* storageManager) {
storage = storageManager;

    SPI.begin(RFID_CLK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN);
    delay(100);

    for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
        mfrc522[reader].PCD_Init(ssPins[reader], RFID_RST_PIN);
        delay(100);
        debugLog("[RFID] Reader %d initialized (SS=%d)", reader, ssPins[reader]);
    }

    delay(500);

    if (storage) {
        registeredCardsCount = storage->loadRegisteredCards(authorizedCards, MAX_REGISTERED_CARDS);
    }

    if (registeredCardsCount == 0) {
        registerNewCard(0x12345678);
        registerNewCard(0x87654321);
        registerNewCard(0xDEADBEEF);
        debugLog("[RFID] Default cards registered");
    }

}

bool RFIDManager::scanReader(RFIDScanType type) {
uint8_t reader = (type == RFID_ENTRY) ? 0 : 1;

    if (!mfrc522[reader].PICC_IsNewCardPresent()) return false;

    delay(10);

    if (!mfrc522[reader].PICC_ReadCardSerial()) return false;

    lastScannedUID = 0;
    for (byte i = 0; i < mfrc522[reader].uid.size; i++) {
        lastScannedUID = (lastScannedUID << 8) | mfrc522[reader].uid.uidByte[i];
    }

    lastScanType = type;

    mfrc522[reader].PICC_HaltA();
    mfrc522[reader].PCD_StopCrypto1();

    debugLog("[RFID_%s] Card: %08X", type == RFID_ENTRY ? "ENTRY" : "EXIT", lastScannedUID);

    return true;

}

uint32_t RFIDManager::getLastScannedUID() const {
return lastScannedUID;
}

void RFIDManager::registerNewCard(uint32_t uid) {
if (registeredCardsCount >= MAX_REGISTERED_CARDS) return;
if (cardExists(uid)) return;

    CardData newCard;
    newCard.uid = uid;
    newCard.timestamp = millis() / 1000;
    newCard.authorized = true;
    newCard.isInside = false;
    memset(newCard.name, 0, sizeof(newCard.name));
    snprintf(newCard.name, sizeof(newCard.name), "CARD_%08X", uid);

    authorizedCards[registeredCardsCount++] = newCard;

    if (storage) {
        storage->saveRegisteredCards(authorizedCards, registeredCardsCount);
    }

    debugLog("[RFID] Card registered: %08X", uid);

}

void RFIDManager::deleteCard(uint32_t uid) {
for (uint16_t i = 0; i < registeredCardsCount; i++) {
if (authorizedCards[i].uid == uid) {
for (uint16_t j = i; j < registeredCardsCount - 1; j++) {
authorizedCards[j] = authorizedCards[j + 1];
}
registeredCardsCount--;
if (storage) {
storage->saveRegisteredCards(authorizedCards, registeredCardsCount);
}
debugLog("[RFID] Card deleted: %08X", uid);
return;
}
}
}

void RFIDManager::listAllCards() {
Serial.println("\n[RFID] Authorized Cards:");
for (uint16_t i = 0; i < registeredCardsCount; i++) {
Serial.printf("[%d] UID: %08X | %s\n", i+1, authorizedCards[i].uid,
authorizedCards[i].name[0] ? authorizedCards[i].name : "(no name)");
}
}

AccessStatus RFIDManager::verifyAccess(uint32_t uid) {
CardData\* card = getCardByUID(uid);
if (card == nullptr) return CARD_UNKNOWN;
return card->authorized ? ACCESS_GRANTED : ACCESS_DENIED;
}

CardData\* RFIDManager::getCardByUID(uint32_t uid) {
for (uint16_t i = 0; i < registeredCardsCount; i++) {
if (authorizedCards[i].uid == uid) {
return &authorizedCards[i];
}
}
return nullptr;
}

CardData\* RFIDManager::getAuthorizedCards() {
return authorizedCards;
}

uint16_t RFIDManager::getRegisteredCardsCount() const {
return registeredCardsCount;
}

bool RFIDManager::cardExists(uint32_t uid) {
return getCardByUID(uid) != nullptr;
}

bool RFIDManager::testConnection() {
return true;
}

# ========== FICHIER COMPLET: src/motion/motion.cpp ==========

#include "motion.h"

PIRMotionSensor::PIRMotionSensor(uint8_t pin)
: sensorPin(pin), lastMotionTime(0), motionConfirmationTime(0),
motionDetected(false), consecutiveHighReads(0) {
}

void PIRMotionSensor::init() {
pinMode(sensorPin, INPUT);
consecutiveHighReads = 0;
motionDetected = false;
lastMotionTime = millis();

    debugLog("[MOTION] PIR sensor initialized on pin %d", sensorPin);

}

void PIRMotionSensor::update() {
bool rawMotion = digitalRead(sensorPin) == HIGH;

    if (rawMotion) {
        if (consecutiveHighReads < REQUIRED_CONSECUTIVE_READS) {
            consecutiveHighReads++;
        }

        if (consecutiveHighReads == REQUIRED_CONSECUTIVE_READS) {
            motionConfirmationTime = millis();
        }
    } else {
        consecutiveHighReads = 0;
    }

}

bool PIRMotionSensor::wasMotionDetected() {
if (consecutiveHighReads >= REQUIRED_CONSECUTIVE_READS &&
(millis() - lastMotionTime) > MOTION_COOLDOWN_MS) {

        lastMotionTime = millis();
        motionDetected = true;
        consecutiveHighReads = 0;
        return true;
    }

    return false;

}

bool PIRMotionSensor::isMotionRaw() const {
return digitalRead(sensorPin) == HIGH;
}

bool PIRMotionSensor::isMotionDebounced() const {
return consecutiveHighReads >= REQUIRED_CONSECUTIVE_READS;
}

void PIRMotionSensor::reset() {
consecutiveHighReads = 0;
motionDetected = false;
lastMotionTime = millis();
}

uint32_t PIRMotionSensor::getTimeSinceLastMotion() const {
return millis() - lastMotionTime;
}

void PIRMotionSensor::test() {
uint32_t testStart = millis();
while (millis() - testStart < 5000) {
update();
if (wasMotionDetected()) {
Serial.println("[MOTION] Detected!");
}
delay(50);
}
debugLog("[MOTION] Test complete");
}

# ========== FICHIER COMPLET: src/button/button.cpp ==========

#include "button.h"

ButtonManager::ButtonManager() {
buttons[0] = {BUTTON_MODE_PIN, HIGH, HIGH, 0, false};
buttons[1] = {BUTTON_LAMP_PIN, HIGH, HIGH, 0, false};
}

void ButtonManager::init() {
pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);
pinMode(BUTTON_LAMP_PIN, INPUT_PULLUP);

    debugLog("[BUTTON] Manager initialized");

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
debugLog("[BUTTON] Test - press buttons");
delay(3000);
}

# ========== FICHIER COMPLET: src/logic/logic.cpp (key functions) ==========

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

    systemStatus->relays.q0_lamp_inside = (systemStatus->counters.presence_count > 0);
    systemStatus->relays.q1_lamp_outside = (systemStatus->daynight == STATUS_NIGHT &&
                                           systemStatus->counters.presence_count == 0);

    updateRelayOutputs();

}

void SystemLogic::updateSystemState() {
if (systemStatus == nullptr) return;

    if (systemStatus->intrusion_detected) {
        if (millis() - lastIntrusionAlert > 5000) {
            lastIntrusionAlert = millis();

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

# ========== FICHIER COMPLET: src/main.cpp (sections clés) ==========

// Dans la mouche de motion detection
if (globalSystemStatus.daynight == STATUS_NIGHT &&
globalSystemStatus.counters.presence_count == 0 &&
motionSensor.wasMotionDetected()) {

    if (millis() - lastMotionTrigger > MOTION_COOLDOWN_MS) {
        lastMotionTrigger = millis();
        globalSystemStatus.intrusion_detected = true;
        storageManager.saveSystemState(&globalSystemStatus);
    }

}

// Dans la boucle des boutons
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
debugLog("[BUTTON] Lamp toggled");
}

// Dans la boucle RFID
if (rfidManager.scanReader(RFID_ENTRY)) {
uint32_t cardUID = rfidManager.getLastScannedUID();
systemLogic.handleRFIDScan(RFID_ENTRY, cardUID);
}

if (rfidManager.scanReader(RFID_EXIT)) {
uint32_t cardUID = rfidManager.getLastScannedUID();
systemLogic.handleRFIDScan(RFID_EXIT, cardUID);
}

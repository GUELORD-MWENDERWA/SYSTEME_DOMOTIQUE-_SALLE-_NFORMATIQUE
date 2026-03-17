#include "storage.h"

StorageManager::StorageManager() : initialized(false) {
}

void StorageManager::init() {
    EEPROM.begin(EEPROM_SIZE);
    initialized = true;
    debugLog("Storage Manager initialized (EEPROM: %d bytes)", EEPROM_SIZE);
}

struct PersistedSystemState {
    uint32_t magic;
    uint16_t version;
    SystemStatus state;
} __attribute__((packed));

void StorageManager::saveSystemState(const SystemStatus* state) {
    if (!initialized || state == nullptr) return;

    PersistedSystemState data;
    data.magic = EEPROM_MAGIC;
    data.version = EEPROM_VERSION;
    data.state = *state;

    EEPROM.put(EEPROM_STATE_ADDR, data);
    EEPROM.commit();

    uint8_t relayMask = 0;
    relayMask |= state->relays.q0_lamp_inside ? (1 << 0) : 0;
    relayMask |= state->relays.q1_lamp_outside ? (1 << 1) : 0;
    relayMask |= state->relays.q2_prise1 ? (1 << 2) : 0;
    relayMask |= state->relays.q3_prise2 ? (1 << 3) : 0;
    relayMask |= state->relays.q4_fan ? (1 << 4) : 0;
    relayMask |= state->relays.q5_buzzer ? (1 << 5) : 0;
    relayMask |= state->relays.q6_led_red ? (1 << 6) : 0;
    relayMask |= state->relays.q7_led_green ? (1 << 7) : 0;

    debugLog("System state saved to EEPROM (mode=%d, relays=0x%02X)", state->current_mode, relayMask);
}

bool StorageManager::loadSystemState(SystemStatus* state) {
    if (!initialized || state == nullptr) return false;

    PersistedSystemState data = {};  // Initialize to zero to avoid uninitialized warnings
    EEPROM.get(EEPROM_STATE_ADDR, data);

    if (data.magic != EEPROM_MAGIC || data.version != EEPROM_VERSION) {
        debugLog("No valid persisted state found (magic=0x%08X, version=%d)", data.magic, data.version);
        return false;
    }

    *state = data.state;

    uint8_t relayMask = 0;
    relayMask |= state->relays.q0_lamp_inside ? (1 << 0) : 0;
    relayMask |= state->relays.q1_lamp_outside ? (1 << 1) : 0;
    relayMask |= state->relays.q2_prise1 ? (1 << 2) : 0;
    relayMask |= state->relays.q3_prise2 ? (1 << 3) : 0;
    relayMask |= state->relays.q4_fan ? (1 << 4) : 0;
    relayMask |= state->relays.q5_buzzer ? (1 << 5) : 0;
    relayMask |= state->relays.q6_led_red ? (1 << 6) : 0;
    relayMask |= state->relays.q7_led_green ? (1 << 7) : 0;

    debugLog("System state loaded from EEPROM (mode=%d, relays=0x%02X)", state->current_mode, relayMask);
    return true;
}

void StorageManager::saveRegisteredCards(const CardData* cards, uint16_t count) {
    if (!initialized || cards == nullptr) return;

    uint16_t limitedCount = count;
    if (limitedCount > MAX_REGISTERED_CARDS) limitedCount = MAX_REGISTERED_CARDS;
    uint16_t addr = EEPROM_CARD_BASE_ADDR;

    EEPROM.put(addr, limitedCount);
    addr += sizeof(uint16_t);

    for (uint16_t i = 0; i < limitedCount; i++) {
        EEPROM.put(addr, cards[i]);
        addr += sizeof(CardData);
    }

    // Clear the remaining slots so that old data doesn't linger
    for (uint16_t i = limitedCount; i < MAX_REGISTERED_CARDS; i++) {
        CardData empty;
        empty.uid = 0xFFFFFFFF;
        empty.timestamp = 0;
        empty.authorized = false;
        empty.isInside = false;
        memset(empty.name, 0, sizeof(empty.name));
        EEPROM.put(addr, empty);
        addr += sizeof(CardData);
    }

    EEPROM.commit();
    debugLog("%d cards saved to EEPROM", limitedCount);
}

uint16_t StorageManager::loadRegisteredCards(CardData* cards, uint16_t maxCount) {
    if (!initialized || cards == nullptr || maxCount == 0) return 0;

    uint16_t addr = EEPROM_CARD_BASE_ADDR;
    uint16_t storedCount = 0;

    EEPROM.get(addr, storedCount);
    addr += sizeof(uint16_t);

    if (storedCount > MAX_REGISTERED_CARDS) {
        storedCount = MAX_REGISTERED_CARDS;
    }

    uint16_t loadedCount = 0;
    for (uint16_t i = 0; i < storedCount && loadedCount < maxCount; i++) {
        CardData card;
        EEPROM.get(addr, card);
        addr += sizeof(CardData);

        // A UID of 0xFFFFFFFF indicates empty slot
        if (card.uid == 0xFFFFFFFF) continue;

        // Ensure name string is null-terminated
        card.name[sizeof(card.name)-1] = '\0';
        cards[loadedCount++] = card;
    }

    debugLog("%d cards loaded from EEPROM", loadedCount);
    return loadedCount;
}

void StorageManager::saveCard(const CardData* card, uint16_t index) {
    if (!initialized || card == nullptr || index >= MAX_REGISTERED_CARDS) return;

    // Load existing card list
    CardData cards[MAX_REGISTERED_CARDS];
    uint16_t count = loadRegisteredCards(cards, MAX_REGISTERED_CARDS);

    // Ensure list is large enough
    for (uint16_t i = count; i <= index; i++) {
        if (i >= MAX_REGISTERED_CARDS) break;
        cards[i].uid = 0xFFFFFFFF;
        cards[i].timestamp = 0;
        cards[i].authorized = false;
    }

    cards[index] = *card;
    if (index >= count) count = index + 1;

    saveRegisteredCards(cards, count);
    debugLog("Card #%d saved", index);
}

void StorageManager::loadCard(CardData* card, uint16_t index) {
    if (!initialized || card == nullptr || index >= MAX_REGISTERED_CARDS) return;

    CardData cards[MAX_REGISTERED_CARDS];
    uint16_t count = loadRegisteredCards(cards, MAX_REGISTERED_CARDS);
    if (index < count) {
        *card = cards[index];
    } else {
        card->uid = 0xFFFFFFFF;
        card->timestamp = 0;
        card->authorized = false;
        card->isInside = false;
        memset(card->name, 0, sizeof(card->name));
    }
}

void StorageManager::saveEnergyHistory(const EnergyData data) {
    if (!initialized) return;

    uint16_t addr = EEPROM_ENERGY_ADDR;
    EEPROM.put(addr, data);
    EEPROM.commit();

    debugLog("Energy snapshot saved");
}

void StorageManager::addLog(const char* logEntry) {
    if (!initialized) return;
    debugLog("Log entry: %s", logEntry);
}

void StorageManager::formatEEPROM() {
    if (!initialized) return;
    
    for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    
    debugLog("EEPROM formatted");
}

void StorageManager::writeString(uint16_t addr, const String& data, uint16_t maxLen) {
    if (!initialized) return;
    
    // Write length first (1 byte)
    uint8_t len = min((uint8_t)data.length(), (uint8_t)maxLen);
    EEPROM.write(addr, len);
    
    // Write string data
    for (uint8_t i = 0; i < len; i++) {
        EEPROM.write(addr + 1 + i, data[i]);
    }
    
    EEPROM.commit();
    debugLog("String written to EEPROM at 0x%04X (len=%d)", addr, len);
}

String StorageManager::readString(uint16_t addr, uint16_t maxLen) {
    if (!initialized) return "";
    
    // Read length first
    uint8_t len = EEPROM.read(addr);
    
    if (len == 0 || len == 0xFF || len > maxLen) {
        return "";
    }
    
    // Read string data
    String result = "";
    for (uint8_t i = 0; i < len; i++) {
        result += (char)EEPROM.read(addr + 1 + i);
    }
    
    debugLog("String read from EEPROM at 0x%04X (len=%d)", addr, len);
    return result;
}
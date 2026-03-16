#include "rfid.h"

RFIDManager::RFIDManager()
    : storage(nullptr), lastScannedUID(0), registeredCardsCount(0) {
    // Reader 0 (ENTRY) uses SS on pin 17 (RFID_SS1_PIN)
    // Reader 1 (EXIT) uses SS on pin 5 (RFID_SS2_PIN)
    ssPins[0] = RFID_SS1_PIN;   // ENTRY on pin 17
    ssPins[1] = RFID_SS2_PIN;   // EXIT on pin 5
}

void RFIDManager::init(StorageManager* storageManager) {
    storage = storageManager;

    SPI.begin(RFID_CLK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN);
    delay(100);

    for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
        pinMode(ssPins[reader], OUTPUT);
        digitalWrite(ssPins[reader], HIGH);  // Deselect initially
        delay(50);
        
        mfrc522[reader].PCD_Init(ssPins[reader], RFID_RST_PIN);
        delay(100);
        
        debugLog("RFID Reader %d initialized (SS:%d)", reader, ssPins[reader]);
    }

    delay(500);

    if (storage) {
        registeredCardsCount = storage->loadRegisteredCards(authorizedCards, MAX_REGISTERED_CARDS);
    }

    if (registeredCardsCount == 0) {
        registerNewCard(0x12345678);
        registerNewCard(0x87654321);
        registerNewCard(0xDEADBEEF);
        debugLog("RFID default cards registered (first boot)");
    }
}

bool RFIDManager::scanReader(RFIDScanType type) {
    uint8_t reader = (type == RFID_ENTRY) ? 0 : 1;
    
    if (!mfrc522[reader].PICC_IsNewCardPresent()) return false;
    if (!mfrc522[reader].PICC_ReadCardSerial()) return false;
    
    lastScannedUID = 0;
    for (byte i = 0; i < mfrc522[reader].uid.size; i++) {
        lastScannedUID = (lastScannedUID << 8) | mfrc522[reader].uid.uidByte[i];
    }
    
    lastScanType = type;
    
    mfrc522[reader].PICC_HaltA();
    mfrc522[reader].PCD_StopCrypto1();
    
    debugLog("RFID %s scanned: %08X", type == RFID_ENTRY ? "ENTRY" : "EXIT", lastScannedUID);
    
    return true;
}

uint32_t RFIDManager::getLastScannedUID() const {
    return lastScannedUID;
}

void RFIDManager::registerNewCard(uint32_t uid) {
    if (registeredCardsCount >= MAX_REGISTERED_CARDS) {
        debugLog("ERROR: Database full!");
        return;
    }

    if (cardExists(uid)) {
        debugLog("Card already registered: %08X", uid);
        return;
    }

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

    debugLog("Card registered: %08X", uid);
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

            debugLog("Card deleted: %08X", uid);
            return;
        }
    }
}

void RFIDManager::listAllCards() {
    Serial.println("\n╔─ CARTES AUTORISÉES ────────────────────╗");
    Serial.print("║ Total: ");
    Serial.print(registeredCardsCount);
    Serial.println("                              ║");
    Serial.println("║─────────────────────────────────────────║");
    
    for (uint16_t i = 0; i < registeredCardsCount; i++) {
        Serial.print("║ #");
        Serial.print(i + 1);
        Serial.print(" - UID: ");
        Serial.print(uint32ToHex(authorizedCards[i].uid));
        Serial.print(" | ");
        Serial.print(authorizedCards[i].name[0] ? authorizedCards[i].name : "(no name)");
        Serial.println(" ║");
    }
    
    Serial.println("╚─────────────────────────────────────────╝\n");
}

AccessStatus RFIDManager::verifyAccess(uint32_t uid) {
    CardData* card = getCardByUID(uid);
    
    if (card == nullptr) {
        return CARD_UNKNOWN;
    }
    
    return card->authorized ? ACCESS_GRANTED : ACCESS_DENIED;
}

CardData* RFIDManager::getCardByUID(uint32_t uid) {
    for (uint16_t i = 0; i < registeredCardsCount; i++) {
        if (authorizedCards[i].uid == uid) {
            return &authorizedCards[i];
        }
    }
    return nullptr;
}

CardData* RFIDManager::getAuthorizedCards() {
    return authorizedCards;
}

uint16_t RFIDManager::getRegisteredCardsCount() const {
    return registeredCardsCount;
}

bool RFIDManager::cardExists(uint32_t uid) {
    return getCardByUID(uid) != nullptr;
}

bool RFIDManager::testConnection() {
    bool allOK = true;
    
    Serial.println("\n┌─ Testing RFID Readers ─────────────────┐");
    
    for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
        const char* name = (reader == 0) ? "ENTRY (SS2)" : "EXIT (SS1)";
        
        Serial.print("│ Reader ");
        Serial.print(reader);
        Serial.print(" (");
        Serial.print(name);
        Serial.print("): ");
        
        byte version = mfrc522[reader].PCD_ReadRegister(MFRC522::VersionReg);
        
        if (version != 0x91 && version != 0x92) {
            Serial.println("✗ NO RESPONSE");
            allOK = false;
        } else {
            Serial.print("✓ OK (v");
            Serial.print(version, HEX);
            Serial.println(")");
        }
    }
    
    Serial.println("└────────────────────────────────────────┘");
    
    return allOK;
}
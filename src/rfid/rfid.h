#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include "../config.h"
#include "../storage/storage.h"
#include <SPI.h>
#include <MFRC522.h>

class RFIDManager {
private:
    static const uint8_t NR_OF_READERS = 2;
    MFRC522 mfrc522[2];
    uint8_t ssPins[2];

    StorageManager* storage;
    uint32_t lastScannedUID;
    RFIDScanType lastScanType;

    CardData authorizedCards[MAX_REGISTERED_CARDS];
    uint16_t registeredCardsCount;

    bool cardExists(uint32_t uid);

public:
    RFIDManager();

    void init(StorageManager* storageManager);
    bool scanReader(RFIDScanType type);
    uint32_t getLastScannedUID() const;

    void registerNewCard(uint32_t uid);
    void deleteCard(uint32_t uid);
    void listAllCards();
    AccessStatus verifyAccess(uint32_t uid);

    CardData* getCardByUID(uint32_t uid);
    CardData* getAuthorizedCards();
    uint16_t getRegisteredCardsCount() const;

    bool testConnection();
};

#endif
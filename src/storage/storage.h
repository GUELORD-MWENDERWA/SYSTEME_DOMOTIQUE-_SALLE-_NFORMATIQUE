#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "../config.h"
#include <EEPROM.h>

class StorageManager {
private:
    bool initialized;

public:
    StorageManager();

    void init();

    // System state persistence
    void saveSystemState(const SystemStatus* state);
    bool loadSystemState(SystemStatus* state);

    // Card persistence
    void saveRegisteredCards(const CardData* cards, uint16_t count);
    uint16_t loadRegisteredCards(CardData* cards, uint16_t maxCount);

    // Legacy helpers (kept for compatibility)
    void saveCard(const CardData* card, uint16_t index);
    void loadCard(CardData* card, uint16_t index);

    // Misc
    void saveEnergyHistory(const EnergyData data);
    void addLog(const char* logEntry);
    void formatEEPROM();
    
    // WiFi Config
    void writeString(uint16_t addr, const String& data, uint16_t maxLen);
    String readString(uint16_t addr, uint16_t maxLen);
};

#endif
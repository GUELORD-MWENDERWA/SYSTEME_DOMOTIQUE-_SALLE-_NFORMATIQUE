#ifndef ENERGY_H
#define ENERGY_H

#include <Arduino.h>
#include "../config.h"
#include <PZEM004Tv30.h>

class EnergyMonitor {
private:
    PZEM004Tv30* pzem;
    EnergyData lastData;
    bool connected;
    
public:
    EnergyMonitor();
    ~EnergyMonitor();
    
    void init();
    void readValues();
    EnergyData getLastData() const;
    
    bool testConnection();
    void printValues();
    
    float getVoltage() const { return lastData.voltage; }
    float getCurrent() const { return lastData.current; }
    float getPower() const { return lastData.power; }
    float getEnergyTotal() const { return lastData.energy_total; }
    float getFrequency() const { return lastData.frequency; }
    float getPowerFactor() const { return lastData.power_factor; }
    bool isConnected() const { return connected; }
};

#endif
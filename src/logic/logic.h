#ifndef LOGIC_H
#define LOGIC_H

#include <Arduino.h>
#include "../config.h"
#include "../relay/relay.h"
#include "../servo/servo.h"
#include "../rfid/rfid.h"
#include "../energy/energy.h"
#include "../ldr/ldr.h"
#include "../storage/storage.h"

class SystemLogic {
private:
    SystemStatus* systemStatus;
    RelayController* relayCtrl;
    ServoController* servoCtrl;
    RFIDManager* rfidMgr;
    EnergyMonitor* energyMon;
    LDRSensor* ldrSensor;
    StorageManager* storage;

    uint32_t lastIntrusionAlert;
    uint8_t failed_access_counter;
    bool stateDirty;

    void updateLightingLogic();
    void saveStateIfNeeded();

public:
    SystemLogic();

    void init(SystemStatus* status, RelayController* relay, ServoController* servo,
              RFIDManager* rfid, EnergyMonitor* energy, LDRSensor* ldr, StorageManager* storageManager);

    void updateRelayOutputs();
    void handleRFIDScan(RFIDScanType scanType, uint32_t cardUID);
    void setSystemMode(SystemMode newMode);
    void updateDayNightStatus(DayNightStatus status);
    void updateSystemState();

    void executeAccessGrantedScenario();
    void executeAccessDeniedScenario();
};

#endif
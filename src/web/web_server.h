#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "logic/logic.h"
#include "rfid/rfid.h"
#include "relay/relay.h"
#include "energy/energy.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <EEPROM.h>

class WebServerManager {
private:
    AsyncWebServer* server;
    SystemStatus* systemStatus;
    SystemLogic* systemLogic;
    RFIDManager* rfidManager;
    RelayController* relayController;
    EnergyMonitor* energyMonitor;
    StorageManager* storageManager;
    bool isAPMode = false;

public:
    WebServerManager();
    
    void init(
        SystemStatus* status,
        SystemLogic* logic,
        RFIDManager* rfid,
        RelayController* relay,
        EnergyMonitor* energy,
        StorageManager* storage
    );
    
    void begin();
    void setupRoutes();
    void setAPMode(bool apMode) { isAPMode = apMode; }
    
private:
    // API Handlers
    void handleGetSystem(AsyncWebServerRequest* request);
    void handleToggleRelay(AsyncWebServerRequest* request, uint32_t relay);
    void handleGetRFIDList(AsyncWebServerRequest* request);
    void handleRegisterRFID(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleDeleteRFID(AsyncWebServerRequest* request, String uid);
    void handleCommand(AsyncWebServerRequest* request, String cmd);
    void handleConfigWiFi(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleConfigWiFiGET(AsyncWebServerRequest* request, String ssid, String password);
    void handleNotFound(AsyncWebServerRequest* request);
};

#endif

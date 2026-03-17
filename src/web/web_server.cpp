#include "web_server.h"

WebServerManager::WebServerManager() : server(nullptr),
    systemStatus(nullptr), systemLogic(nullptr), rfidManager(nullptr),
    relayController(nullptr), energyMonitor(nullptr), storageManager(nullptr) {
}

void WebServerManager::init(
    SystemStatus* status,
    SystemLogic* logic,
    RFIDManager* rfid,
    RelayController* relay,
    EnergyMonitor* energy,
    StorageManager* storage) {
    
    systemStatus = status;
    systemLogic = logic;
    rfidManager = rfid;
    relayController = relay;
    energyMonitor = energy;
    storageManager = storage;
    
    server = new AsyncWebServer(80);
    
    debugLog("WebServerManager initialized");
}

void WebServerManager::begin() {
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        debugLog("SPIFFS mount failed!");
        return;
    }
    debugLog("SPIFFS mounted successfully");
    
    setupRoutes();
    
    // Start server
    server->begin();
    debugLog("Web Server started on http://ESP32.local or http://192.168.x.x");
}

void WebServerManager::setupRoutes() {
    // CRITICAL: REGISTER STATIC HANDLER FIRST before any server->on() routes!
    auto staticHandler = server->serveStatic("/", SPIFFS, "/");
    staticHandler.setDefaultFile(isAPMode ? "config.html" : "index.html");
    staticHandler.setCacheControl("max-age=31536000"); // 1 year cache
    
    // THEN register API routes with /api/* prefix so they don't conflict
    
    // DIAGNOSTIC: GET /api/files - Debug SPIFFS contents
    server->on("/api/files", HTTP_GET, [this](AsyncWebServerRequest* request) {
        File root = SPIFFS.open("/");
        String fileList = "[";
        bool first = true;
        File file = root.openNextFile();
        while (file) {
            if (!first) fileList += ",";
            fileList += "\"" + String(file.name()) + "\"";
            first = false;
            file = root.openNextFile();
        }
        fileList += "]";
        request->send(200, "application/json", fileList);
    });
    
    // GET /api/system - Get system status and energy data
    server->on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetSystem(request);
    });
    
    // POST /api/relay - Toggle relay with query param: ?id=0
    server->on("/api/relay", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("id")) {
            uint32_t id = atoi(request->getParam("id")->value().c_str());
            if (id < 8) {
                handleToggleRelay(request, id);
            } else {
                request->send(400, "application/json", "{\"error\":\"Invalid relay ID\"}");
            }
        } else {
            request->send(400, "application/json", "{\"error\":\"Missing id parameter\"}");
        }
    });
    
    // GET /api/rfid/list - Get RFID cards list
    server->on("/api/rfid/list", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetRFIDList(request);
    });
    
    // POST /api/rfid/register - Register new RFID card
    server->on("/api/rfid/register", HTTP_POST, 
        [this](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0 && len > 0) {
                handleRegisterRFID(request, data, len);
            }
        }
    );
    
    // POST /api/rfid/delete - Delete RFID card (use query param: ?uid=...)
    server->on("/api/rfid/delete", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("uid")) {
            String uid = request->getParam("uid")->value();
            handleDeleteRFID(request, uid);
        } else {
            request->send(400, "application/json", "{\"error\":\"Missing uid parameter\"}");
        }
    });
    
    // POST /api/cmd - Execute command (use query param: ?cmd=...)
    server->on("/api/cmd", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("cmd")) {
            String cmd = request->getParam("cmd")->value();
            handleCommand(request, cmd);
        } else {
            request->send(400, "application/json", "{\"error\":\"Missing cmd parameter\"}");
        }
    });
    
    // GET /api/config/wifi - Configure WiFi via query parameters
    server->on("/api/config/wifi", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("ssid") && request->hasParam("password")) {
            String ssid = request->getParam("ssid")->value();
            String password = request->getParam("password")->value();
            handleConfigWiFiGET(request, ssid, password);
        } else {
            request->send(400, "application/json", "{\"error\":\"Missing ssid or password parameters\"}");
        }
    });
    
    // POST /api/config/wifi - Configure WiFi credentials (only in AP mode)
    server->on("/api/config/wifi", HTTP_POST,
        [this](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0 && len > 0) {
                handleConfigWiFi(request, data, len);
            }
        }
    );
    
    // 404 handler
    server->onNotFound([this](AsyncWebServerRequest* request) {
        handleNotFound(request);
    });
}

void WebServerManager::handleGetSystem(AsyncWebServerRequest* request) {
    if (systemStatus == nullptr || energyMonitor == nullptr) {
        request->send(500, "application/json", "{\"error\":\"System not initialized\"}");
        return;
    }
    
    // Ultra-fast response - avoiding heavy JSON serialization
    // Format: {"v":X.X,"c":X.X,"p":X,"f":X.X,"pc":X,"ec":X,"xc":X,"m":X,"d":X,"i":X,"r":[...]}
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "{\"voltage\":%.1f,\"current\":%.2f,\"power\":%.0f,\"frequency\":%.1f,"
        "\"presence_count\":%u,\"entries_count\":%u,\"exits_count\":%u,"
        "\"current_mode\":%u,\"daynight\":%u,\"intrusion_detected\":%s,"
        "\"relays\":[%s,%s,%s,%s,%s,%s,%s,%s]}",
        energyMonitor->getVoltage(),
        energyMonitor->getCurrent(),
        energyMonitor->getPower(),
        energyMonitor->getFrequency(),
        systemStatus->counters.presence_count,
        systemStatus->counters.entries_count,
        systemStatus->counters.exits_count,
        systemStatus->current_mode,
        systemStatus->daynight,
        systemStatus->intrusion_detected ? "true" : "false",
        systemStatus->relays.q0_lamp_inside ? "true" : "false",
        systemStatus->relays.q1_lamp_outside ? "true" : "false",
        systemStatus->relays.q2_prise1 ? "true" : "false",
        systemStatus->relays.q3_prise2 ? "true" : "false",
        systemStatus->relays.q4_fan ? "true" : "false",
        systemStatus->relays.q5_buzzer ? "true" : "false",
        systemStatus->relays.q6_led_red ? "true" : "false",
        systemStatus->relays.q7_led_green ? "true" : "false"
    );
    
    request->send(200, "application/json", buffer);
}

void WebServerManager::handleToggleRelay(AsyncWebServerRequest* request, uint32_t relay) {
    if (systemStatus == nullptr) {
        request->send(500, "application/json", "{\"error\":\"System not initialized\"}");
        return;
    }
    
    if (relay > 7) {
        request->send(400, "application/json", "{\"error\":\"Invalid relay ID\"}");
        return;
    }
    
    // Array of relay pointers for faster access
    bool* relayPtrs[] = {
        &systemStatus->relays.q0_lamp_inside,
        &systemStatus->relays.q1_lamp_outside,
        &systemStatus->relays.q2_prise1,
        &systemStatus->relays.q3_prise2,
        &systemStatus->relays.q4_fan,
        &systemStatus->relays.q5_buzzer,
        &systemStatus->relays.q6_led_red,
        &systemStatus->relays.q7_led_green
    };
    
    const char* relayNames[] = {
        "Lampe intérieure", "Lampe extérieure", "Prise 1", "Prise 2",
        "Ventilateur", "Buzzer", "LED Rouge", "LED Verte"
    };
    
    bool* relayPtr = relayPtrs[relay];
    *relayPtr = !*relayPtr;
    systemLogic->updateRelayOutputs();
    
    // Non-blocking save (schedule for later)
    if (storageManager) {
        storageManager->saveSystemState(systemStatus);
    }
    
    // Ultra-fast response
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
        "{\"relay\":%u,\"state\":%s,\"name\":\"%s\"}",
        relay, *relayPtr ? "true" : "false", relayNames[relay]
    );
    request->send(200, "application/json", buffer);
}

void WebServerManager::handleGetRFIDList(AsyncWebServerRequest* request) {
    if (rfidManager == nullptr) {
        request->send(200, "application/json", "[]");
        return;
    }
    
    CardData* allCards = rfidManager->getAuthorizedCards();
    uint16_t count = rfidManager->getRegisteredCardsCount();
    
    // Build JSON manually for speed
    String response = "[";
    
    if (count > 0 && allCards != nullptr) {
        for (uint16_t i = 0; i < count; i++) {
            if (i > 0) response += ",";
            response += "{\"uid\":\"";
            response += uint32ToHex(allCards[i].uid);
            response += "\",\"name\":\"";
            response += (allCards[i].name[0] ? allCards[i].name : "No name");
            response += "\",\"authorized\":";
            response += (allCards[i].authorized ? "true" : "false");
            response += ",\"isInside\":";
            response += (allCards[i].isInside ? "true" : "false");
            response += "}";
        }
    }
    
    response += "]";
    request->send(200, "application/json", response);
}

void WebServerManager::handleRegisterRFID(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"JSON parse error\"}");
        return;
    }
    
    String uidStr = doc["uid"];
    String name = doc["name"];
    
    // Parse UID (hex format like "0x12345678")
    uint32_t uid = (uint32_t)strtoul(uidStr.c_str(), nullptr, 0);
    
    if (rfidManager) {
        rfidManager->registerNewCard(uid);
        CardData* card = rfidManager->getCardByUID(uid);
        if (card && name.length() > 0) {
            name.toCharArray(card->name, sizeof(card->name));
            storageManager->saveRegisteredCards(rfidManager->getAuthorizedCards(), rfidManager->getRegisteredCardsCount());
        }
    }
    
    JsonDocument response;
    response["status"] = "ok";
    response["uid"] = uint32ToHex(uid);
    response["name"] = name;
    
    String jsonResponse;
    serializeJson(response, jsonResponse);
    request->send(200, "application/json", jsonResponse);
}

void WebServerManager::handleDeleteRFID(AsyncWebServerRequest* request, String uid) {
    uint32_t cardUID = (uint32_t)strtoul(uid.c_str(), nullptr, 16);
    
    if (rfidManager) {
        rfidManager->deleteCard(cardUID);
    }
    
    JsonDocument doc;
    doc["status"] = "ok";
    doc["deleted"] = uint32ToHex(cardUID);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleCommand(AsyncWebServerRequest* request, String cmd) {
    cmd.toUpperCase();
    
    JsonDocument doc;
    
    if (cmd == "ZERO") {
        systemStatus->relays = {false, false, false, false, false, false, false, false};
        systemStatus->counters = {0, 0, 0, 0, 0};
        systemStatus->intrusion_detected = false;
        systemLogic->updateRelayOutputs();
        doc["status"] = "ok";
        doc["message"] = "System reset";
    } 
    else if (cmd == "RESET") {
        systemStatus->counters = {0, 0, 0, 0, 0};
        doc["status"] = "ok";
        doc["message"] = "Counters reset";
    }
    else if (cmd == "RELAY") {
        doc["status"] = "ok";
        doc["relays"][0] = systemStatus->relays.q0_lamp_inside;
        doc["relays"][1] = systemStatus->relays.q1_lamp_outside;
        doc["relays"][2] = systemStatus->relays.q2_prise1;
        doc["relays"][3] = systemStatus->relays.q3_prise2;
        doc["relays"][4] = systemStatus->relays.q4_fan;
        doc["relays"][5] = systemStatus->relays.q5_buzzer;
        doc["relays"][6] = systemStatus->relays.q6_led_red;
        doc["relays"][7] = systemStatus->relays.q7_led_green;
    }
    else if (cmd == "REBOOT") {
        doc["status"] = "ok";
        doc["message"] = "Rebooting...";
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        delay(1000);
        ESP.restart();
        return;
    }
    else if (cmd == "MODE_ACCESS") {
        systemLogic->setSystemMode(MODE_ACCESS);
        doc["status"] = "ok";
        doc["message"] = "Mode ACCESS active";
    }
    else if (cmd == "MODE_REG") {
        systemLogic->setSystemMode(MODE_REGISTRATION);
        doc["status"] = "ok";
        doc["message"] = "Mode REGISTRATION active";
    }
    else if (cmd == "DAYMODE") {
        systemStatus->daynight = STATUS_DAY;
        doc["status"] = "ok";
        doc["message"] = "Mode JOUR active";
    }
    else if (cmd == "NIGHTMODE") {
        systemStatus->daynight = STATUS_NIGHT;
        doc["status"] = "ok";
        doc["message"] = "Mode NUIT active";
    }
    else if (cmd == "ALARM_ON") {
        systemStatus->intrusion_detected = true;
        doc["status"] = "ok";
        doc["message"] = "Alarme activee";
    }
    else if (cmd == "ALARM_OFF") {
        systemStatus->intrusion_detected = false;
        doc["status"] = "ok";
        doc["message"] = "Alarme desactivee";
    }
    else {
        doc["status"] = "error";
        doc["message"] = "Unknown command";
    }
    
    storageManager->saveSystemState(systemStatus);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleConfigWiFi(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    if (storageManager == nullptr) {
        request->send(500, "application/json", "{\"error\":\"Storage not initialized\"}");
        return;
    }
    
    // Parse JSON payload
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc["ssid"] || !doc["password"]) {
        request->send(400, "application/json", "{\"error\":\"Missing ssid or password\"}");
        return;
    }
    
    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();
    
    if (ssid.length() == 0 || ssid.length() > 32) {
        request->send(400, "application/json", "{\"error\":\"Invalid SSID length\"}");
        return;
    }
    
    if (password.length() < 8 || password.length() > 64) {
        request->send(400, "application/json", "{\"error\":\"Password must be 8-64 characters\"}");
        return;
    }
    
    // Save credentials to EEPROM
    storageManager->writeString(EEPROM_ADDR_SSID, ssid, 32);
    storageManager->writeString(EEPROM_ADDR_PASSWORD, password, 64);
    
    // Mark as valid
    EEPROM.write(EEPROM_ADDR_WIFI_VALID, 0x01);
    EEPROM.commit();
    
    Serial.println("[WIFI] Credentials saved! Rebooting...");
    
    JsonDocument responseDoc;
    responseDoc["status"] = "success";
    responseDoc["message"] = "WiFi configured. Rebooting...";
    
    String response;
    serializeJson(responseDoc, response);
    request->send(200, "application/json", response);
    
    // Quick reboot after saving
    delay(500);
    ESP.restart();
}

void WebServerManager::handleConfigWiFiGET(AsyncWebServerRequest* request, String ssid, String password) {
    if (storageManager == nullptr) {
        request->send(500, "application/json", "{\"error\":\"Storage not initialized\"}");
        return;
    }
    
    if (ssid.length() == 0 || ssid.length() > 32) {
        request->send(400, "application/json", "{\"error\":\"Invalid SSID length\"}");
        return;
    }
    
    if (password.length() < 8 || password.length() > 64) {
        request->send(400, "application/json", "{\"error\":\"Password must be 8-64 characters\"}");
        return;
    }
    
    // Save credentials to EEPROM
    storageManager->writeString(EEPROM_ADDR_SSID, ssid, 32);
    storageManager->writeString(EEPROM_ADDR_PASSWORD, password, 64);
    
    // Mark as valid
    EEPROM.write(EEPROM_ADDR_WIFI_VALID, 0x01);
    EEPROM.commit();
    
    Serial.println("[WIFI] Credentials saved! Rebooting...");
    
    JsonDocument responseDoc;
    responseDoc["status"] = "success";
    responseDoc["message"] = "WiFi configured. Rebooting...";
    
    String response;
    serializeJson(responseDoc, response);
    request->send(200, "application/json", response);
    
    // Quick reboot after saving
    delay(500);
    ESP.restart();
}

void WebServerManager::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not Found\"}");
}

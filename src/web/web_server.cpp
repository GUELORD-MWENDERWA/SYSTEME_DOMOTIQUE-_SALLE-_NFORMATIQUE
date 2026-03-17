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
    // API Routes - Define BEFORE static file serving
    
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
    
    // Serve static files from SPIFFS (MUST be last)
    server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    
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
    
    JsonDocument doc;
    
    // Energy data
    doc["voltage"] = energyMonitor->getVoltage();
    doc["current"] = energyMonitor->getCurrent();
    doc["power"] = energyMonitor->getPower();
    doc["frequency"] = energyMonitor->getFrequency();
    
    // System status
    doc["presence_count"] = systemStatus->counters.presence_count;
    doc["entries_count"] = systemStatus->counters.entries_count;
    doc["exits_count"] = systemStatus->counters.exits_count;
    doc["current_mode"] = systemStatus->current_mode;
    doc["daynight"] = systemStatus->daynight;
    doc["intrusion_detected"] = systemStatus->intrusion_detected;
    
    // Relays
    JsonArray relays = doc["relays"].to<JsonArray>();
    relays.add(systemStatus->relays.q0_lamp_inside);
    relays.add(systemStatus->relays.q1_lamp_outside);
    relays.add(systemStatus->relays.q2_prise1);
    relays.add(systemStatus->relays.q3_prise2);
    relays.add(systemStatus->relays.q4_fan);
    relays.add(systemStatus->relays.q5_buzzer);
    relays.add(systemStatus->relays.q6_led_red);
    relays.add(systemStatus->relays.q7_led_green);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
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
    
    JsonDocument doc;
    doc["relay"] = relay;
    doc["state"] = *relayPtr;
    doc["name"] = relayNames[relay];
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleGetRFIDList(AsyncWebServerRequest* request) {
    if (rfidManager == nullptr) {
        request->send(500, "application/json", "[]");
        return;
    }
    
    JsonDocument doc;
    JsonArray cards = doc.to<JsonArray>();
    
    CardData* allCards = rfidManager->getAuthorizedCards();
    uint16_t count = rfidManager->getRegisteredCardsCount();
    
    for (uint16_t i = 0; i < count; i++) {
        JsonObject card = cards.add<JsonObject>();
        card["uid"] = uint32ToHex(allCards[i].uid);
        card["name"] = allCards[i].name[0] ? allCards[i].name : "No name";
        card["authorized"] = allCards[i].authorized;
        card["isInside"] = allCards[i].isInside;
    }
    
    String response;
    serializeJson(doc, response);
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
        request->send(200, "application/json");
        delay(1000);
        ESP.restart();
        return;
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

void WebServerManager::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not Found\"}");
}

#include "serial_cmd.h"
#include "../energy/energy.h"
#include "../rfid/rfid.h"
#include "../relay/relay.h"
#include "../servo/servo.h"
#include "../ldr/ldr.h"
#include "../logic/logic.h"

extern SystemStatus globalSystemStatus;
extern EnergyMonitor energyMonitor;
extern RFIDManager rfidManager;
extern RelayController relayController;
extern ServoController servoController;
extern LDRSensor ldrSensor;
extern StorageManager storageManager;
extern SystemLogic systemLogic;

SerialCommandHandler::SerialCommandHandler() {
}

void SerialCommandHandler::init() {
    debugLog("Serial Command Handler initialized at %d bps", SERIAL_BAUDRATE);
    delay(100);
    printMainMenu();
}

void SerialCommandHandler::processCommand() {
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                Serial.println();
                parseCommand(inputBuffer);
                inputBuffer = "";
                Serial.print("> ");
            }
        } else if (c == '\b' || c == 0x7F) {
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                Serial.write(c);
            }
        } else if (c >= 32 && c <= 126) {
            inputBuffer += c;
            Serial.write(c);
        }
    }
}

void SerialCommandHandler::parseCommand(const String& cmd) {
    String mainCmd = cmd;
    mainCmd.toUpperCase();
    
    int spacePos = cmd.indexOf(' ');
    if (spacePos > 0) {
        mainCmd = cmd.substring(0, spacePos);
        mainCmd.toUpperCase();
    }
    
    String params = spacePos > 0 ? cmd.substring(spacePos + 1) : "";
    
    Serial.println();
    
    // ─────────────────────────────────────────
    // ÉNERGIE
    // ─────────────────────────────────────────
    if (mainCmd == "E") {
        energyMonitor.printValues();
    }
    else if (mainCmd == "ER") {
        handleEnergyReport();
    }
    
    // ─────────────────────────────────────────
    // RFID & ACCÈS
    // ─────────────────────────────────────────
    else if (mainCmd == "RLIST") {
        rfidManager.listAllCards();
    }
    else if (mainCmd == "RDEL") {
        if (params.length() == 0) {
            Serial.println("Usage: RDEL <UID hex>");
        } else {
            uint32_t uid = strtoul(params.c_str(), nullptr, 16);
            rfidManager.deleteCard(uid);
            Serial.print("✓ Carte supprimée: ");
            Serial.println(uint32ToHex(uid));
        }
    }
    else if (mainCmd == "REDIT") {
        int sep = params.indexOf(' ');
        if (sep < 0) {
            Serial.println("Usage: REDIT <oldUID> <newUID>");
        } else {
            uint32_t oldUid = strtoul(params.substring(0, sep).c_str(), nullptr, 16);
            uint32_t newUid = strtoul(params.substring(sep + 1).c_str(), nullptr, 16);
            CardData* card = rfidManager.getCardByUID(oldUid);
            if (card == nullptr) {
                Serial.println("❌ Carte introuvable");
            } else {
                card->uid = newUid;
                storageManager.saveRegisteredCards(rfidManager.getAuthorizedCards(), rfidManager.getRegisteredCardsCount());
                Serial.print("✓ UID modifié: ");
                Serial.print(uint32ToHex(oldUid));
                Serial.print(" -> ");
                Serial.println(uint32ToHex(newUid));
            }
        }
    }
    else if (mainCmd == "RNAME") {
        int sep = params.indexOf(' ');
        if (sep < 0) {
            Serial.println("Usage: RNAME <UID> <name>");
        } else {
            uint32_t uid = strtoul(params.substring(0, sep).c_str(), nullptr, 16);
            String name = params.substring(sep + 1);
            CardData* card = rfidManager.getCardByUID(uid);
            if (card == nullptr) {
                Serial.println("❌ Carte introuvable");
            } else {
                name.toCharArray(card->name, sizeof(card->name));
                storageManager.saveRegisteredCards(rfidManager.getAuthorizedCards(), rfidManager.getRegisteredCardsCount());
                Serial.print("✓ Nom modifié pour ");
                Serial.print(uint32ToHex(uid));
                Serial.print(": ");
                Serial.println(card->name);
            }
        }
    }
    else if (mainCmd == "RMODE") {
        Serial.print("Mode actuel: ");
        Serial.println(globalSystemStatus.current_mode == MODE_ACCESS ? "ACCESS" : "REGISTRATION");
    }
    else if (mainCmd == "RREG") {
        systemLogic.setSystemMode(MODE_REGISTRATION);
        Serial.println("✓ Mode enregistrement activé");
    }
    else if (mainCmd == "RACC") {
        systemLogic.setSystemMode(MODE_ACCESS);
        Serial.println("✓ Mode accès activé");
    }
    
    // ─────────────────────────────────────────
    // RELAIS & ÉCLAIRAGE
    // ─────────────────────────────────────────
    else if (mainCmd == "L0") {
        if (params == "1") {
            globalSystemStatus.relays.q0_lamp_inside = true;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Lampe intérieure: ON");
            storageManager.saveSystemState(&globalSystemStatus);
        } else if (params == "0") {
            globalSystemStatus.relays.q0_lamp_inside = false;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Lampe intérieure: OFF");
            storageManager.saveSystemState(&globalSystemStatus);
        } else {
            Serial.println("❌ Paramètre invalide. Usage: L0 1/0");
        }
    }
    else if (mainCmd == "L1") {
        if (params == "1") {
            globalSystemStatus.relays.q1_lamp_outside = true;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Lampe extérieure: ON");
            storageManager.saveSystemState(&globalSystemStatus);
        } else if (params == "0") {
            globalSystemStatus.relays.q1_lamp_outside = false;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Lampe extérieure: OFF");
            storageManager.saveSystemState(&globalSystemStatus);
        } else {
            Serial.println("❌ Paramètre invalide. Usage: L1 1/0");
        }
    }
    else if (mainCmd == "P1") {
        if (params == "1") {
            globalSystemStatus.relays.q2_prise1 = true;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Prise 1: ON");
            storageManager.saveSystemState(&globalSystemStatus);
        } else if (params == "0") {
            globalSystemStatus.relays.q2_prise1 = false;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Prise 1: OFF");
            storageManager.saveSystemState(&globalSystemStatus);
        } else {
            Serial.println("❌ Paramètre invalide. Usage: P1 1/0");
        }
    }
    else if (mainCmd == "P2") {
        if (params == "1") {
            globalSystemStatus.relays.q3_prise2 = true;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Prise 2: ON");
            storageManager.saveSystemState(&globalSystemStatus);
        } else if (params == "0") {
            globalSystemStatus.relays.q3_prise2 = false;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Prise 2: OFF");
            storageManager.saveSystemState(&globalSystemStatus);
        } else {
            Serial.println("❌ Paramètre invalide. Usage: P2 1/0");
        }
    }
    else if (mainCmd == "F") {
        if (params == "1") {
            globalSystemStatus.relays.q4_fan = true;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Ventilateur: ON");
            storageManager.saveSystemState(&globalSystemStatus);
        } else if (params == "0") {
            globalSystemStatus.relays.q4_fan = false;
            systemLogic.updateRelayOutputs();
            Serial.println("✓ Ventilateur: OFF");
            storageManager.saveSystemState(&globalSystemStatus);
        } else {
            Serial.println("❌ Paramètre invalide. Usage: F 1/0");
        }
    }
    
    // ─────────────────────────────────────────
    // DIAGNOSTIC & TEST
    // ─────────────────────────────────────────
    else if (mainCmd == "D") {
        handleDiagnostic();
    }
    else if (mainCmd == "T") {
        handleTest();
    }
    else if (mainCmd == "C") {
        Serial.println("╔─ COMPTEURS PRÉSENCE ──────────────┐");
        Serial.print("│ Entrées:  ");
        Serial.print(globalSystemStatus.counters.entries_count);
        Serial.println("                    │");
        Serial.print("│ Sorties:  ");
        Serial.print(globalSystemStatus.counters.exits_count);
        Serial.println("                    │");
        Serial.print("│ Présence: ");
        Serial.print(globalSystemStatus.counters.presence_count);
        Serial.println("                    │");
        Serial.println("└────────────────────────────────────┘");
    }
    
    // ─────────────────────────────────────────
    // SYSTÈME
    // ─────────────────────────────────────────
    else if (mainCmd == "HELP") {
        printHelpMenu();
    }
    else if (mainCmd == "INFO") {
        Serial.println("╔════════════════════════════════════╗");
        Serial.println("║  DOMOTIQUE SALLE INFORMATIQUE      ║");
        Serial.println("║  Plateforme: ESP32 DevKit          ║");
        Serial.println("║  Version: 1.0.0                    ║");
        Serial.println("║  Compile Date: " __DATE__ "        ║");
        Serial.print("║  Uptime: ");
        uint32_t uptime = millis() / 1000;
        uint16_t hours = uptime / 3600;
        uint16_t minutes = (uptime % 3600) / 60;
        uint16_t seconds = uptime % 60;
        Serial.print(hours);
        Serial.print("h ");
        Serial.print(minutes);
        Serial.print("m ");
        Serial.print(seconds);
        Serial.println("s              ║");
        Serial.println("╚════════════════════════════════════╝");
    }
    else if (mainCmd == "STATE") {
        Serial.println("╔─ ÉTAT SYSTÈME ─────────────────────┐");
        Serial.print("│ Mode: ");
        Serial.println(globalSystemStatus.current_mode == MODE_ACCESS ? 
                      "ACCESS            │" : "REGISTRATION      │");
        Serial.print("│ État FSM: ");
        switch(globalSystemStatus.state) {
            case STATE_IDLE:
                Serial.println("IDLE              │");
                break;
            case STATE_CARD_SCANNED:
                Serial.println("CARD_SCANNED      │");
                break;
            case STATE_ACCESS_GRANTED:
                Serial.println("ACCESS_GRANTED    │");
                break;
            case STATE_ACCESS_DENIED:
                Serial.println("ACCESS_DENIED     │");
                break;
            case STATE_NIGHT_MODE:
                Serial.println("NIGHT_MODE        │");
                break;
            case STATE_INTRUSION_ALERT:
                Serial.println("INTRUSION_ALERT   │");
                break;
            default:
                Serial.println("UNKNOWN           │");
        }
        Serial.print("│ Jour/Nuit: ");
        Serial.println(globalSystemStatus.daynight == STATUS_DAY ? 
                      "JOUR              │" : "NUIT              │");
        Serial.println("└────────────────────────────────────┘");
    }
    else if (mainCmd == "RELAY") {
        Serial.println("╔─ ÉTAT RELAIS 74HC595 ──────────────┐");
        Serial.print("│ Q0 (Lampe int):  ");
        Serial.println(relayController.getOutputState(Q0_LAMP_INSIDE) ? "ON  │" : "OFF │");
        Serial.print("│ Q1 (Lampe ext):  ");
        Serial.println(relayController.getOutputState(Q1_LAMP_OUTSIDE) ? "ON  │" : "OFF │");
        Serial.print("│ Q2 (Prise 1):    ");
        Serial.println(relayController.getOutputState(Q2_PRISE1) ? "ON  │" : "OFF │");
        Serial.print("│ Q3 (Prise 2):    ");
        Serial.println(relayController.getOutputState(Q3_PRISE2) ? "ON  │" : "OFF │");
        Serial.print("│ Q4 (Fan):        ");
        Serial.println(relayController.getOutputState(Q4_FAN) ? "ON  │" : "OFF │");
        Serial.print("│ Q5 (Buzzeur):    ");
        Serial.println(relayController.getOutputState(Q5_BUZZER) ? "ON  │" : "OFF │");
        Serial.print("│ Q6 (LED Rouge):  ");
        Serial.println(relayController.getOutputState(Q6_LED_RED) ? "ON  │" : "OFF │");
        Serial.print("│ Q7 (LED Verte):  ");
        Serial.println(relayController.getOutputState(Q7_LED_GREEN) ? "ON  │" : "OFF │");
        Serial.println("└────────────────────────────────────┘");
    }
    else if (mainCmd == "SERVO") {
        Serial.print("Servo position: ");
        Serial.print(servoController.getCurrentAngle());
        Serial.println("°");
        
        Serial.print("Testing servo... ");
        servoController.setAngle(90);
        delay(500);
        servoController.setAngle(0);
        Serial.println("Done");
    }
    else if (mainCmd == "REBOOT") {
        Serial.println("\n╔════════════════════════════════════╗");
        Serial.println("║       Redémarrage en cours...      ║");
        Serial.println("╚════════════════════════════════════╝\n");
        delay(1000);
        ESP.restart();
    }
    else if (mainCmd == "RESET") {
        Serial.println("Réinitialisation compteurs...");
        globalSystemStatus.counters.entries_count = 0;
        globalSystemStatus.counters.exits_count = 0;
        globalSystemStatus.counters.presence_count = 0;
        globalSystemStatus.counters.total_access_attempts = 0;
        globalSystemStatus.counters.denied_access_count = 0;
        
        // Also reset card "isInside" states
        CardData* cards = rfidManager.getAuthorizedCards();
        for (uint16_t i = 0; i < rfidManager.getRegisteredCardsCount(); i++) {
            cards[i].isInside = false;
        }
        storageManager.saveRegisteredCards(cards, rfidManager.getRegisteredCardsCount());
        storageManager.saveSystemState(&globalSystemStatus);
        Serial.println("✓ Compteurs et états cartes réinitialisés");
    }
    else if (mainCmd == "ZERO") {
        Serial.println("Mise à zéro complète (relais, compteurs, alarmes)...");
        
        // Reset all relays
        globalSystemStatus.relays.q0_lamp_inside = false;
        globalSystemStatus.relays.q1_lamp_outside = false;
        globalSystemStatus.relays.q2_prise1 = false;
        globalSystemStatus.relays.q3_prise2 = false;
        globalSystemStatus.relays.q4_fan = false;
        globalSystemStatus.relays.q5_buzzer = false;
        globalSystemStatus.relays.q6_led_red = false;
        globalSystemStatus.relays.q7_led_green = false;
        systemLogic.updateRelayOutputs();
        
        // Reset counters
        globalSystemStatus.counters.entries_count = 0;
        globalSystemStatus.counters.exits_count = 0;
        globalSystemStatus.counters.presence_count = 0;
        globalSystemStatus.counters.total_access_attempts = 0;
        globalSystemStatus.counters.denied_access_count = 0;
        
        // Reset alarms and state
        globalSystemStatus.intrusion_detected = false;
        globalSystemStatus.state = STATE_IDLE;
        globalSystemStatus.last_rfid_scan = 0;
        globalSystemStatus.state_transition_time = 0;
        
        storageManager.saveSystemState(&globalSystemStatus);
        Serial.println("✓ Tout remis à zéro");
    }
    else if (mainCmd == "FORMAT") {
        Serial.println("Formatage de l'EEPROM... (toutes les données sauvegardées seront perdues)");
        storageManager.formatEEPROM();

        // Reset the system state to defaults and save it
        globalSystemStatus.current_mode = MODE_ACCESS;
        globalSystemStatus.state = STATE_IDLE;
        globalSystemStatus.daynight = STATUS_DAY;
        globalSystemStatus.counters = {0, 0, 0, 0, 0};
        globalSystemStatus.relays = {false, false, false, false, false, false, false, false};
        globalSystemStatus.intrusion_detected = false;
        globalSystemStatus.last_rfid_scan = 0;
        globalSystemStatus.state_transition_time = 0;

        systemLogic.updateRelayOutputs();
        storageManager.saveSystemState(&globalSystemStatus);
        Serial.println("✓ EEPROM formatée et état remis à zéro. Redémarrez pour initialiser le système.");
    }
    else if (mainCmd == "BUZ") {
        if (params == "") {
            relayController.setOutput(Q5_BUZZER, true);
            delay(200);
            relayController.setOutput(Q5_BUZZER, false);
            Serial.println("✓ Buzzeur: 1 bip");
        } else if (params == "2") {
            for (int i = 0; i < 2; i++) {
                relayController.setOutput(Q5_BUZZER, true);
                delay(100);
                relayController.setOutput(Q5_BUZZER, false);
                delay(100);
            }
            Serial.println("✓ Buzzeur: 2 bips");
        } else if (params == "3") {
            for (int i = 0; i < 3; i++) {
                relayController.setOutput(Q5_BUZZER, true);
                delay(200);
                relayController.setOutput(Q5_BUZZER, false);
                delay(200);
            }
            Serial.println("✓ Buzzeur: 3 bips longs");
        }
    }
    else if (mainCmd == "SIGNAL") {
        Serial.print("Signalisation... ");
        
        // LED verte
        relayController.setOutput(Q7_LED_GREEN, true);
        delay(200);
        relayController.setOutput(Q7_LED_GREEN, false);
        delay(200);
        
        // LED rouge
        relayController.setOutput(Q6_LED_RED, true);
        delay(200);
        relayController.setOutput(Q6_LED_RED, false);
        
        Serial.println("Done");
    }
    else {
        Serial.println("❌ Commande inconnue. Tapez 'HELP' pour l'aide.");
    }
    
    Serial.print("> ");
}

void SerialCommandHandler::handleDiagnostic() {
    Serial.println("╔─ DIAGNOSTIC SYSTÈME ──────────────────┐");
    
    Serial.print("│ Mode: ");
    Serial.print(globalSystemStatus.current_mode == MODE_ACCESS ? "ACCESS" : "REGISTRATION");
    Serial.println("          │");
    
    Serial.print("│ État FSM: ");
    Serial.print(globalSystemStatus.state);
    Serial.println("                      │");
    
    Serial.print("│ Jour/Nuit: ");
    Serial.print(globalSystemStatus.daynight == STATUS_DAY ? "JOUR" : "NUIT");
    Serial.println("                 │");
    
    Serial.print("│ Présence: ");
    Serial.print(globalSystemStatus.counters.presence_count);
    Serial.println("                   │");
    
    Serial.print("│ Entrées: ");
    Serial.print(globalSystemStatus.counters.entries_count);
    Serial.print(" | Sorties: ");
    Serial.print(globalSystemStatus.counters.exits_count);
    Serial.println("      │");
    
    Serial.print("│ LDR value: ");
    Serial.print(ldrSensor.getLastValue());
    Serial.println("                 │");
    
    Serial.print("│ Intrusion: ");
    Serial.print(globalSystemStatus.intrusion_detected ? "OUI" : "NON");
    Serial.println("                   │");
    
    Serial.print("│ Tension PZEM: ");
    Serial.print(energyMonitor.getVoltage(), 1);
    Serial.println(" V           │");
    
    Serial.print("│ Courant PZEM: ");
    Serial.print(energyMonitor.getCurrent(), 2);
    Serial.println(" A          │");
    
    Serial.print("│ Puissance PZEM: ");
    Serial.print(energyMonitor.getPower(), 1);
    Serial.println(" W         │");
    
    Serial.println("└────────────────────────────────────────┘");
}

void SerialCommandHandler::handleTest() {
    Serial.println("\n╔─ AUTO-TEST COMPOSANTS ─────────────────┐\n");
    
    // Test Relais
    Serial.print("├─ 74HC595 (Relais): ");
    relayController.test();
    Serial.println();
    
    // Test Servo
    Serial.print("├─ Servo Motor: ");
    servoController.test();
    Serial.println();
    
    // Test LDR
    Serial.print("├─ LDR Sensor: ");
    ldrSensor.test();
    Serial.println();
    
    // Test PZEM
    Serial.print("├─ PZEM-004T: ");
    if (energyMonitor.testConnection()) {
        Serial.println("✓ OK");
    } else {
        Serial.println("⚠ Vérifier connexion");
    }
    
    Serial.println("\n└─ AUTO-TEST TERMINÉ ──────────────────┘\n");
}

void SerialCommandHandler::handleEnergyReport() {
    Serial.println("╔─ RAPPORT ÉNERGÉTIQUE ─────────────────┐");
    
    EnergyData data = energyMonitor.getLastData();
    
    Serial.print("│ Tension moyenne: ");
    Serial.print(data.voltage, 1);
    Serial.println(" V          │");
    
    Serial.print("│ Courant: ");
    Serial.print(data.current, 3);
    Serial.println(" A             │");
    
    Serial.print("│ Puissance: ");
    Serial.print(data.power, 1);
    Serial.println(" W             │");
    
    Serial.print("│ Consommation: ");
    Serial.print(data.energy_total, 2);
    Serial.println(" kWh         │");
    
    Serial.print("│ Facteur de puissance: ");
    Serial.print(data.power_factor, 2);
    Serial.println("         │");
    
    Serial.print("│ Fréquence: ");
    Serial.print(data.frequency, 1);
    Serial.println(" Hz             │");
    
    Serial.println("└────────────────────────────────────────┘");
}

void SerialCommandHandler::printMainMenu() {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  CONSOLE DOMOTIQUE SALLE INFORMATIQUE  ║");
    Serial.println("║  Tapez 'HELP' pour les commandes      ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    Serial.print("> ");
}

void SerialCommandHandler::printHelpMenu() {
    Serial.println("\n┌─ COMMANDES DISPONIBLES ───────────────────┐\n");
    
    Serial.println("📊 ÉNERGIE:");
    Serial.println("  e         - Mesures PZEM actuelles");
    Serial.println("  er        - Rapport énergétique\n");
    
    Serial.println("🔑 RFID & ACCÈS:");
    Serial.println("  rlist     - Liste cartes autorisées");
    Serial.println("  rdel <uid> - Supprimer une carte (hex)");
    Serial.println("  redit <old> <new> - Modifier UID (hex)");
    Serial.println("  rname <uid> <nom> - Renommer une carte");
    Serial.println("  rmode     - Affiche mode actuel");
    Serial.println("  rreg      - Mode enregistrement");
    Serial.println("  racc      - Mode accès\n");
    
    Serial.println("💡 RELAIS & ÉCLAIRAGE:");
    Serial.println("  l0 1/0    - Lampe intérieure ON/OFF");
    Serial.println("  l1 1/0    - Lampe extérieure ON/OFF");
    Serial.println("  p1 1/0    - Prise 1 ON/OFF");
    Serial.println("  p2 1/0    - Prise 2 ON/OFF");
    Serial.println("  f 1/0     - Ventilateur ON/OFF\n");
    
    Serial.println("🔧 DIAGNOSTIC & TEST:");
    Serial.println("  d         - Diagnostic système");
    Serial.println("  t         - Test tous composants");
    Serial.println("  c         - Compteurs présence");
    Serial.println("  state     - État FSM");
    Serial.println("  relay     - État relais 74HC595");
    Serial.println("  servo     - Test servo moteur");
    Serial.println("  buz [n]   - Buzzeur (1/2/3 bips)");
    Serial.println("  signal    - Test signalisation");
    Serial.println("  format    - Formate EEPROM + réinitialise l'état\n");
    
    Serial.println("ℹ️  SYSTÈME:");
    Serial.println("  info      - Infos système");
    Serial.println("  reset     - Réinit compteurs");
    Serial.println("  zero      - Tout mettre à zéro (relais, compteurs, alarmes)");
    Serial.println("  reboot    - Redémarrage ESP32");
    Serial.println("  help      - Ce menu\n");
    
    Serial.println("└──────────────────────────────────────────┘\n");
    Serial.print("> ");
}
#include "energy.h"

EnergyMonitor::EnergyMonitor() : pzem(nullptr), connected(false) {
    memset(&lastData, 0, sizeof(EnergyData));
}

EnergyMonitor::~EnergyMonitor() {
    if (pzem != nullptr) {
        delete pzem;
    }
}

void EnergyMonitor::init() {
    Serial2.begin(PZEM_BAUDRATE, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
    delay(100);
    
    pzem = new PZEM004Tv30(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
    delay(500);
    
    connected = true;
    debugLog("PZEM-004T v3.0 initialized on Serial2");
}

void EnergyMonitor::readValues() {
    if (pzem == nullptr) return;

    float voltage = pzem->voltage();
    float current = pzem->current();
    float power = pzem->power();
    float energy = pzem->energy();
    float frequency = pzem->frequency();
    float pf = pzem->pf();

    // If the sensor is disconnected or responding incorrectly, reset to 0
    bool sensorOk = true;

    if (isnan(voltage) || voltage < 0.0f || voltage > 300.0f) {
        sensorOk = false;
    }
    if (isnan(current) || current < 0.0f || current > 100.0f) {
        sensorOk = false;
    }
    if (isnan(power) || power < 0.0f || power > 5000.0f) {
        sensorOk = false;
    }
    if (isnan(frequency) || frequency < 0.0f || frequency > 100.0f) {
        sensorOk = false;
    }
    if (isnan(pf) || pf < -1.0f || pf > 1.0f) {
        sensorOk = false;
    }

    if (!sensorOk) {
        lastData.voltage = 0.0f;
        lastData.current = 0.0f;
        lastData.power = 0.0f;
        lastData.energy_total = 0.0f;
        lastData.frequency = 0.0f;
        lastData.power_factor = 0.0f;
        connected = false;
    } else {
        lastData.voltage = voltage;
        lastData.current = current;
        lastData.power = power;
        lastData.energy_total = energy / 1000.0f;
        lastData.frequency = frequency;
        lastData.power_factor = pf;
        connected = true;
    }

    lastData.timestamp = millis() / 1000;
}


bool EnergyMonitor::testConnection() {
    if (pzem == nullptr) return false;
    
    float voltage = pzem->voltage();
    return !isnan(voltage);
}

EnergyData EnergyMonitor::getLastData() const {
    return lastData;
}

void EnergyMonitor::printValues() {
    Serial.println("\n╔─ ÉNERGIE PZEM-004T ────────────────╗");
    Serial.print("║ Tension:    ");
    Serial.print(lastData.voltage, 1);
    Serial.println(" V          ║");
    Serial.print("║ Courant:    ");
    Serial.print(lastData.current, 3);
    Serial.println(" A         ║");
    Serial.print("║ Puissance:  ");
    Serial.print(lastData.power, 1);
    Serial.println(" W        ║");
    Serial.print("║ Énergie:    ");
    Serial.print(lastData.energy_total, 2);
    Serial.println(" kWh       ║");
    Serial.println("╚─────────────────────────────────────╝\n");
}

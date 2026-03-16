#include "ldr.h"

LDRSensor::LDRSensor() : lastValue(0), lastReadTime(0) {
}

void LDRSensor::init() {
    pinMode(LDR_PIN, INPUT);
    debugLog("LDR sensor initialized on pin %d (ADC)", LDR_PIN);
}

uint16_t LDRSensor::readValue() {
    lastValue = analogRead(LDR_PIN);
    lastReadTime = millis();
    return lastValue;
}

uint16_t LDRSensor::getLastValue() const {
    return lastValue;
}

bool LDRSensor::isDayTime() {
    return lastValue >= LDR_DAY_THRESHOLD;
}

bool LDRSensor::isNight() {
    return lastValue < LDR_DAY_THRESHOLD;
}

void LDRSensor::test() {
    Serial.println("\n╔─ TEST LDR ────────────────────────┐");
    Serial.print("│ LDR Value (raw ADC): ");
    Serial.print(readValue());
    Serial.println("              │");
    Serial.print("│ Seuil jour/nuit: ");
    Serial.print(LDR_DAY_THRESHOLD);
    Serial.println("                   │");
    Serial.print("│ État actuel: ");
    Serial.print(isDayTime() ? "JOUR" : "NUIT");
    Serial.println("                       │");
    Serial.println("└─────────────────────────────────────┘\n");
}
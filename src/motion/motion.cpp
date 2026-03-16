#include "motion.h"

PIRMotionSensor::PIRMotionSensor(uint8_t pin)
    : sensorPin(pin), lastMotionTime(0), motionConfirmationTime(0),
      motionDetected(false), consecutiveHighReads(0) {
}

void PIRMotionSensor::init() {
    // Use INPUT instead of INPUT_PULLDOWN to avoid floating pin issues
    pinMode(sensorPin, INPUT);
    consecutiveHighReads = 0;
    motionDetected = false;
    lastMotionTime = millis();
    
    debugLog("PIR Motion Sensor initialized on pin %d (INPUT mode)", sensorPin);
}

void PIRMotionSensor::update() {
    bool rawMotion = digitalRead(sensorPin) == HIGH;
    
    if (rawMotion) {
        // Increment consecutive HIGH reads counter
        if (consecutiveHighReads < REQUIRED_CONSECUTIVE_READS) {
            consecutiveHighReads++;
        }
        
        // If we just reached the required consecutive reads, mark motion detection time
        if (consecutiveHighReads == REQUIRED_CONSECUTIVE_READS) {
            motionConfirmationTime = millis();
        }
    } else {
        // Reset counter on any LOW read (debounce)
        consecutiveHighReads = 0;
    }
}

bool PIRMotionSensor::wasMotionDetected() {
    // Check if we have confirmed motion detection (via consecutive reads)
    // AND enough time has passed since last motion (cooldown)
    if (consecutiveHighReads >= REQUIRED_CONSECUTIVE_READS &&
        (millis() - lastMotionTime) > MOTION_COOLDOWN_MS) {
        
        lastMotionTime = millis();
        motionDetected = true;
        consecutiveHighReads = 0;  // Reset counter after detection
        return true;
    }
    
    return false;
}

bool PIRMotionSensor::isMotionRaw() const {
    return digitalRead(sensorPin) == HIGH;
}

bool PIRMotionSensor::isMotionDebounced() const {
    return consecutiveHighReads >= REQUIRED_CONSECUTIVE_READS;
}

void PIRMotionSensor::reset() {
    consecutiveHighReads = 0;
    motionDetected = false;
    lastMotionTime = millis();
}

uint32_t PIRMotionSensor::getTimeSinceLastMotion() const {
    return millis() - lastMotionTime;
}

void PIRMotionSensor::test() {
    Serial.println("\n╔─ TEST PIR MOTION SENSOR ──────────┐");
    Serial.println("│ Raw pin state will be monitored... │");
    Serial.println("│ Move near the sensor to test       │");
    Serial.println("└────────────────────────────────────┘\n");
    
    uint32_t testStart = millis();
    while (millis() - testStart < 5000) {
        update();
        
        Serial.print("Raw: ");
        Serial.print(isMotionRaw() ? "HIGH" : "LOW");
        Serial.print(" | Debounced: ");
        Serial.print(isMotionDebounced() ? "YES" : "NO");
        Serial.print(" | Consecutive reads: ");
        Serial.println(consecutiveHighReads);
        
        if (wasMotionDetected()) {
            Serial.println("✓ MOTION EVENT DETECTED!");
        }
        
        delay(50);
    }
    
    debugLog("PIR test complete");
}

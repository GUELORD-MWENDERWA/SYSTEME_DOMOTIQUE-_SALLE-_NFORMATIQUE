#ifndef MOTION_H
#define MOTION_H

#include <Arduino.h>
#include "../config.h"

/**
 * PIR Motion Sensor Manager
 * 
 * Provides robust motion detection with:
 * - Hardware debouncing via consistent reads
 * - Cooldown to prevent false re-triggers
 * - Only active during night mode
 * - Two-stage validation: pin HIGH + time confirmation
 */
class PIRMotionSensor {
private:
    uint8_t sensorPin;
    uint32_t lastMotionTime;
    uint32_t motionConfirmationTime;
    bool motionDetected;
    uint8_t consecutiveHighReads;
    static const uint8_t REQUIRED_CONSECUTIVE_READS = 8;  // ~80ms debounce (10ms reads)

public:
    PIRMotionSensor(uint8_t pin = MOTION_SENSOR_PIN);

    void init();
    
    /**
     * Update sensor state - call frequently (every 10ms or in main loop)
     */
    void update();
    
    /**
     * Check if motion was just detected (valid detection event)
     * Returns true once and clears the event
     */
    bool wasMotionDetected();
    
    /**
     * Get raw pin state (HIGH = motion, LOW = no motion)
     */
    bool isMotionRaw() const;
    
    /**
     * Get debounced motion state
     */
    bool isMotionDebounced() const;
    
    /**
     * Reset motion state (for testing)
     */
    void reset();
    
    /**
     * Get milliseconds elapsed since last motion was detected
     */
    uint32_t getTimeSinceLastMotion() const;
    
    /**
     * Run interactive test of PIR sensor
     */
    void test();
};

#endif

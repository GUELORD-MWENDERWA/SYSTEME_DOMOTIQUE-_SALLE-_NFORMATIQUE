#ifndef LDR_H
#define LDR_H

#include <Arduino.h>
#include "../config.h"

class LDRSensor {
private:
    uint16_t lastValue;
    uint32_t lastReadTime;
    
public:
    LDRSensor();
    
    void init();
    uint16_t readValue();
    uint16_t getLastValue() const;
    bool isDayTime();
    bool isNight();
    void test();
};

#endif
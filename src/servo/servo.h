#ifndef SERVO_H
#define SERVO_H

#include <Arduino.h>
#include "../config.h"
#include <ESP32Servo.h>

class ServoController {
private:
    Servo servoMotor;
    uint8_t currentAngle;
    
public:
    ServoController();
    
    void init();
    void setAngle(uint8_t angle);
    void open();
    void close();
    uint8_t getCurrentAngle() const;
    void test();
};

#endif
#include "servo.h"

ServoController::ServoController() : currentAngle(0) {
}

void ServoController::init() {
    servoMotor.setPeriodHertz(50);
    servoMotor.attach(SERVO_PIN, 1000, 2000);
    
    setAngle(0);
    debugLog("Servo motor initialized on pin %d", SERVO_PIN);
}

void ServoController::setAngle(uint8_t angle) {
    if (angle > 180) angle = 180;
    
    currentAngle = angle;
    servoMotor.write(angle);
    
    debugLog("Servo angle set to: %d°", angle);
}

void ServoController::open() {
    setAngle(180);
}

void ServoController::close() {
    setAngle(0);
}

uint8_t ServoController::getCurrentAngle() const {
    return currentAngle;
}

void ServoController::test() {
    debugLog("Servo test: 0° -> 90° -> 180° -> 0°");
    
    setAngle(0);
    delay(500);
    setAngle(90);
    delay(500);
    setAngle(180);
    delay(500);
    setAngle(0);
    
    debugLog("Servo test complete");
}
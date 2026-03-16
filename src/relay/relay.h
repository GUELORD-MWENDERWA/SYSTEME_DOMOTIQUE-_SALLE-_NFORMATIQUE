#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "../config.h"

class RelayController {
private:
    uint8_t registerState;

    void updateRegister();

public:
    RelayController();

    void init();

    // Single output control
    void setOutput(uint8_t channel, bool state);

    // Bulk control
    void setAllOn();
    void setAllOff();
    void setBulk(uint8_t mask);

    // Query
    bool getOutputState(uint8_t channel) const;
    uint8_t getMask() const;

    // Testing
    void test();
};

#endif
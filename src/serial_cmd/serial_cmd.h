#ifndef SERIAL_CMD_H
#define SERIAL_CMD_H

#include <Arduino.h>
#include "../config.h"

class SerialCommandHandler {
private:
    String inputBuffer;
    
    void parseCommand(const String& cmd);
    void handleDiagnostic();
    void handleTest();
    void handleEnergyReport();
    
public:
    SerialCommandHandler();
    
    void init();
    void processCommand();
    void printMainMenu();
    void printHelpMenu();
};

#endif
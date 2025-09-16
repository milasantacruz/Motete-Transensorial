#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>

struct PumpState {
    int pumpId;
    bool isActive;
    unsigned long activationStartTime;
    unsigned long cooldownStartTime;
    int activationTime;
    int cooldownTime;
    int cooldownRemaining;
};

class PumpController {
private:
    PumpState pumps[4]; // Máximo 4 bombas
    int pumpCount;
    int pumpPins[4];
    
    void updatePumpState(int pumpId);
    void activatePump(int pumpId);
    void deactivatePump(int pumpId);
    
public:
    PumpController();
    ~PumpController();
    
    void initialize();
    void loop();
    
    bool activatePumpCommand(int pumpId);
    PumpState getPumpState(int pumpId);
    void setPumpConfig(int pumpId, int activationTime, int cooldownTime);
    int getCooldownRemaining(int pumpId);
    bool isPumpAvailable(int pumpId);
};

#endif

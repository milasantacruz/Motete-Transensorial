#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include "config.h"

class PumpController {
private:
    int pumpPins[2];
    bool pumpStates[2];
    int pumpLevels[2];
    
public:
    PumpController();
    void initialize();
    void setPumpState(int pumpId, bool state);
    bool getPumpState(int pumpId);
    void setPumpLevel(int pumpId, int level);
    int getPumpLevel(int pumpId);
    void updatePumps();
};

#endif
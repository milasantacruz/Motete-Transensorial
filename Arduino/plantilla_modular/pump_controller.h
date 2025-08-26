#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include "config.h"
#include <Arduino.h>
class PumpController {
private:
    int* pumpPins;        
    bool* pumpStates;     
    int* pumpLevels;      
    int pumpCount;  // Número real de bombas
    
public:
    PumpController(int count);  // Constructor con parámetro de config.h
    ~PumpController();
    void initialize();
    void setPumpState(int pumpId, bool state);
    bool getPumpState(int pumpId);
    void setPumpLevel(int pumpId, int level);
    int getPumpLevel(int pumpId);
    int getPumpCount() const { return pumpCount; }
    void updatePumps();
};

#endif
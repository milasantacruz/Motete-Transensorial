#include "pump_controller.h"
#include <Arduino.h>

PumpController::PumpController(int count) : pumpCount(count) {
    pumpPins = new int[count];
    pumpStates = new bool[count];
    pumpLevels = new int[count];
    
    for (int i = 0; i < count; i++) {
        pumpPins[i] = deviceConfig.pumpPins[i];  // Pines configurables
        pumpStates[i] = false;
        pumpLevels[i] = 0;
    }
}

PumpController::~PumpController() {
    // Liberar memoria de arrays dinámicos
    delete[] pumpPins;
    delete[] pumpStates;
    delete[] pumpLevels;
}

void PumpController::initialize() {
    Serial.println("�� Inicializando PumpController...");
    
    for (int i = 0; i < pumpCount; i++) {  
        Serial.printf("�� Configurando bomba %d en pin %d\n", i, pumpPins[i]);
        pinMode(pumpPins[i], OUTPUT);
        digitalWrite(pumpPins[i], LOW);
        Serial.printf("✅ Bomba %d configurada\n", i);
    }
    
    Serial.println("✅ PumpController inicializado completamente");
}

void PumpController::setPumpState(int pumpId, bool state) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        pumpStates[pumpId] = state;
        digitalWrite(pumpPins[pumpId], state ? HIGH : LOW);
    }
}

bool PumpController::getPumpState(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        return pumpStates[pumpId];
    }
    return false;
}

void PumpController::setPumpLevel(int pumpId, int level) {
    if (pumpId >= 0 && pumpId < pumpCount && level >= 0 && level <= 100) {
        pumpLevels[pumpId] = level;
    }
}

int PumpController::getPumpLevel(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        return pumpLevels[pumpId];
    }
    return 0;
}

void PumpController::updatePumps() {

}
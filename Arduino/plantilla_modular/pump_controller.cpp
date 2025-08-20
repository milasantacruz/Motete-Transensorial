#include "pump_controller.h"

PumpController::PumpController() {
    // Definir pines de las bombas (ajustar según tu hardware)
    pumpPins[0] = 5;  // GPIO5 para bomba 1
    pumpPins[1] = 4;  // GPIO4 para bomba 2
    
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        pumpStates[i] = false;
        pumpLevels[i] = 0;
    }
}

void PumpController::initialize() {
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        pinMode(pumpPins[i], OUTPUT);
        digitalWrite(pumpPins[i], LOW);
    }
}

void PumpController::setPumpState(int pumpId, bool state) {
    if (pumpId >= 0 && pumpId < deviceConfig.pumpCount) {
        pumpStates[pumpId] = state;
        digitalWrite(pumpPins[pumpId], state ? HIGH : LOW);
    }
}

bool PumpController::getPumpState(int pumpId) {
    if (pumpId >= 0 && pumpId < deviceConfig.pumpCount) {
        return pumpStates[pumpId];
    }
    return false;
}

void PumpController::setPumpLevel(int pumpId, int level) {
    if (pumpId >= 0 && pumpId < deviceConfig.pumpCount && level >= 0 && level <= 100) {
        pumpLevels[pumpId] = level;
    }
}

int PumpController::getPumpLevel(int pumpId) {
    if (pumpId >= 0 && pumpId < deviceConfig.pumpCount) {
        return pumpLevels[pumpId];
    }
    return 0;
}

void PumpController::updatePumps() {
    // Aquí puedes agregar lógica adicional para el control de las bombas
    // Por ejemplo, control PWM para niveles de intensidad
}
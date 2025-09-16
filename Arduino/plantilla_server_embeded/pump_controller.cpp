#include "pump_controller.h"
#include "config.h"

PumpController::PumpController() : pumpCount(0) {
    // Inicializar estado de bombas
    for (int i = 0; i < 4; i++) {
        pumps[i].pumpId = i;
        pumps[i].isActive = false;
        pumps[i].activationStartTime = 0;
        pumps[i].cooldownStartTime = 0;
        pumps[i].activationTime = deviceConfig.pumpDefaults.activationTime;
        pumps[i].cooldownTime = deviceConfig.pumpDefaults.cooldownTime;
        pumps[i].cooldownRemaining = 0;
    }
}

PumpController::~PumpController() {
    // Desactivar todas las bombas al destruir
    for (int i = 0; i < pumpCount; i++) {
        deactivatePump(i);
    }
}

void PumpController::initialize() {
    pumpCount = deviceConfig.pumpCount;
    
    // Copiar configuración de pines
    for (int i = 0; i < pumpCount; i++) {
        pumpPins[i] = deviceConfig.pumpPins[i];
    }
    
    // Configurar pines como salida
    for (int i = 0; i < pumpCount; i++) {
        pinMode(pumpPins[i], OUTPUT);
        digitalWrite(pumpPins[i], LOW); // Inicialmente apagadas
    }
    
    Serial.printf("PumpController inicializado con %d bombas\n", pumpCount);
    for (int i = 0; i < pumpCount; i++) {
        Serial.printf("Bomba %d en pin %d\n", i, pumpPins[i]);
    }
}

void PumpController::loop() {
    // Actualizar estado de todas las bombas
    for (int i = 0; i < pumpCount; i++) {
        updatePumpState(i);
    }
}

void PumpController::updatePumpState(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        return;
    }
    
    PumpState& pump = pumps[pumpId];
    unsigned long currentTime = millis();
    
    // Si la bomba está activa, verificar si debe desactivarse
    if (pump.isActive) {
        if (currentTime - pump.activationStartTime >= pump.activationTime) {
            deactivatePump(pumpId);
            pump.cooldownStartTime = currentTime;
            pump.cooldownRemaining = pump.cooldownTime;
            Serial.printf("Bomba %d desactivada, iniciando cooldown de %d ms\n", 
                         pumpId, pump.cooldownTime);
        }
    }
    
    // Actualizar cooldown restante
    if (pump.cooldownRemaining > 0) {
        unsigned long cooldownElapsed = currentTime - pump.cooldownStartTime;
        pump.cooldownRemaining = max(0, (int)(pump.cooldownTime - cooldownElapsed));
        
        if (pump.cooldownRemaining == 0) {
            Serial.printf("Bomba %d cooldown completado\n", pumpId);
        }
    }
}

bool PumpController::activatePumpCommand(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        Serial.printf("Error: ID de bomba inválido %d\n", pumpId);
        return false;
    }
    
    if (!isPumpAvailable(pumpId)) {
        Serial.printf("Error: Bomba %d no disponible (en cooldown)\n", pumpId);
        return false;
    }
    
    activatePump(pumpId);
    return true;
}

void PumpController::activatePump(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        return;
    }
    
    PumpState& pump = pumps[pumpId];
    
    digitalWrite(pumpPins[pumpId], HIGH);
    pump.isActive = true;
    pump.activationStartTime = millis();
    
    Serial.printf("Bomba %d activada por %d ms\n", pumpId, pump.activationTime);
}

void PumpController::deactivatePump(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        return;
    }
    
    PumpState& pump = pumps[pumpId];
    
    digitalWrite(pumpPins[pumpId], LOW);
    pump.isActive = false;
    
    Serial.printf("Bomba %d desactivada\n", pumpId);
}

PumpState PumpController::getPumpState(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        PumpState emptyState = {0};
        return emptyState;
    }
    
    return pumps[pumpId];
}

void PumpController::setPumpConfig(int pumpId, int activationTime, int cooldownTime) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        return;
    }
    
    pumps[pumpId].activationTime = activationTime;
    pumps[pumpId].cooldownTime = cooldownTime;
    
    Serial.printf("Configuración de bomba %d actualizada: activación=%d ms, cooldown=%d ms\n", 
                 pumpId, activationTime, cooldownTime);
}

int PumpController::getCooldownRemaining(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        return 0;
    }
    
    return pumps[pumpId].cooldownRemaining;
}

bool PumpController::isPumpAvailable(int pumpId) {
    if (pumpId < 0 || pumpId >= pumpCount) {
        return false;
    }
    
    PumpState& pump = pumps[pumpId];
    return !pump.isActive && pump.cooldownRemaining == 0;
}

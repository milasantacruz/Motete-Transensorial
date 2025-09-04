#include "pump_controller.h"
#include <Arduino.h>

PumpController::PumpController(int count) : pumpCount(count) {
    pumpPins = new int[count];
    pumpStates = new bool[count];
    pumpLevels = new int[count];
    pumpActivationTimes = new int[count];
    pumpCooldownTimes = new int[count];
    pumpLastActivation = new unsigned long[count];
    
    for (int i = 0; i < count; i++) {
        pumpPins[i] = deviceConfig.pumpPins[i];  // Pines configurables
        pumpStates[i] = false;
        pumpLevels[i] = 0;
        pumpActivationTimes[i] = 1000;  // 1 segundo por defecto
        pumpCooldownTimes[i] = 5000;    // 5 segundos por defecto
        pumpLastActivation[i] = 0;      // Nunca activado
    }
}

PumpController::~PumpController() {
    // Liberar memoria de arrays dinámicos
    delete[] pumpPins;
    delete[] pumpStates;
    delete[] pumpLevels;
    delete[] pumpActivationTimes;
    delete[] pumpCooldownTimes;
    delete[] pumpLastActivation;
}

void PumpController::initialize() {
    Serial.println("�� Inicializando PumpController...");
    
    for (int i = 0; i < pumpCount; i++) {  
        Serial.printf("�� Configurando bomba %d en pin %d\n", i, pumpPins[i]);
        pinMode(pumpPins[i], OUTPUT);
        digitalWrite(pumpPins[i], LOW);
        // FORZAR reset de estados (por si acaso)
        pumpStates[i] = false;
        pumpLevels[i] = 0;
        
        Serial.printf("✅ Bomba %d configurada - Estado: %s\n", i, pumpStates[i] ? "ACTIVA" : "INACTIVA");
    }
    
    Serial.println("✅ PumpController inicializado completamente");
}

void PumpController::setPumpState(int pumpId, bool state) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        pumpStates[pumpId] = state;

        digitalWrite(pumpPins[pumpId], state ? HIGH : LOW);
        
        // Registrar timestamp de activación
        if (state) {
            pumpLastActivation[pumpId] = millis();
        }
        
        // Log para LEDs de prueba
        Serial.print("💡 LED ");
        Serial.print(pumpId);
        Serial.print(" (pin ");
        Serial.print(pumpPins[pumpId]);
        Serial.print(") ");
        Serial.print(state ? "ENCENDIDO" : "APAGADO");
        Serial.print(" - Estado digital: ");
        Serial.println(digitalRead(pumpPins[pumpId]));
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
    // Verificar si alguna bomba debe desactivarse por tiempo
    for (int i = 0; i < pumpCount; i++) {
        if (pumpStates[i]) {
            unsigned long currentTime = millis();
            unsigned long activationDuration = currentTime - pumpLastActivation[i];
            
            if (activationDuration >= pumpActivationTimes[i]) {
                setPumpState(i, false);
                Serial.printf("⏰ Bomba %d desactivada por tiempo (%lu ms)\n", i, activationDuration);
            }
        }
    }
}

bool PumpController::isPumpAvailable(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        // Verificar si está en cooldown
        unsigned long currentTime = millis();
        unsigned long timeSinceLastActivation = currentTime - pumpLastActivation[pumpId];
        
        // Disponible si no está activa Y no está en cooldown
        return !pumpStates[pumpId] && (timeSinceLastActivation >= pumpCooldownTimes[pumpId]);
    }
    return false;
}

// Métodos para configuración de bombas
void PumpController::setPumpConfig(int pumpId, int activationTime, int cooldownTime) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        pumpActivationTimes[pumpId] = activationTime;
        pumpCooldownTimes[pumpId] = cooldownTime;
        
        Serial.printf("🔧 Bomba %d configurada - Activación: %d ms, Cooldown: %d ms\n", 
                     pumpId, activationTime, cooldownTime);
    }
}

int PumpController::getPumpActivationTime(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        return pumpActivationTimes[pumpId];
    }
    return 0;
}

int PumpController::getPumpCooldownTime(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        return pumpCooldownTimes[pumpId];
    }
    return 0;
}

unsigned long PumpController::getPumpLastActivation(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        return pumpLastActivation[pumpId];
    }
    return 0;
}

void PumpController::resetPumpConfig(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        pumpActivationTimes[pumpId] = 1000;  // 1 segundo por defecto
        pumpCooldownTimes[pumpId] = 5000;    // 5 segundos por defecto
        pumpLastActivation[pumpId] = 0;      // Reset timestamp
        
        Serial.printf("🔄 Configuración de bomba %d restablecida\n", pumpId);
    }
}

void PumpController::resetAllPumpConfigs() {
    for (int i = 0; i < pumpCount; i++) {
        resetPumpConfig(i);
    }
    Serial.println("🔄 Todas las configuraciones de bombas restablecidas");
}
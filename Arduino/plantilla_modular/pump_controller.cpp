#include "pump_controller.h"
#include "network_manager.h"
#include "command_definition.h"
#include <Arduino.h>
#include <ArduinoJson.h>

PumpController::PumpController(int count, NetworkManager* netMgr) 
    : pumpCount(count), networkManager(netMgr), initialConfigSent(false) {
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
        pumpActivationTimes[i] = deviceConfig.pumpDefaults.activationTime;  // Usar configuración
        pumpCooldownTimes[i] = deviceConfig.pumpDefaults.cooldownTime;      // Usar configuración
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

int PumpController::getPumpCooldownRemaining(int pumpId) {
    if (pumpId >= 0 && pumpId < pumpCount) {
        unsigned long currentTime = millis();
        unsigned long timeSinceLastActivation = currentTime - pumpLastActivation[pumpId];
        
        // Si está en cooldown, calcular tiempo restante
        if (timeSinceLastActivation < pumpCooldownTimes[pumpId]) {
            return pumpCooldownTimes[pumpId] - timeSinceLastActivation;
        }
        
        // Si no está en cooldown, retornar 0
        return 0;
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

void PumpController::performInitialMQTTConfig() {
    if (initialConfigSent) {
        Serial.println("⚠️ Configuración inicial MQTT ya enviada");
        return;
    }
    
    if (!networkManager) {
        Serial.println("❌ NetworkManager no disponible para configuración MQTT");
        return;
    }
    
    if (!networkManager->isMQTTConnected()) {
        Serial.println("❌ MQTT no conectado, no se puede enviar configuración");
        return;
    }
    
    Serial.println("🔧 Enviando configuración inicial de bombas vía MQTT...");
    Serial.printf("🔧 Valores por defecto: activación=%dms, cooldown=%dms\n", 
                  deviceConfig.pumpDefaults.activationTime, 
                  deviceConfig.pumpDefaults.cooldownTime);
    
    // Enviar configuración para cada bomba
    for (int i = 0; i < pumpCount; i++) {
        Serial.printf("🔧 Enviando configuración para bomba %d...\n", i);
        sendPumpConfigCommand(i, 
                            deviceConfig.pumpDefaults.activationTime, 
                            deviceConfig.pumpDefaults.cooldownTime);
        delay(100); // Pequeña pausa entre comandos
    }
    
    initialConfigSent = true;
    Serial.println("✅ Configuración inicial MQTT completada");
}

void PumpController::sendPumpConfigCommand(int pumpId, int activationTime, int cooldownTime) {
    // Crear comando set_pump_config
    StaticJsonDocument<256> doc;
    doc["command_id"] = "init_config_" + String(pumpId) + "_" + String(millis());
    doc["action"] = Commands::SET_PUMP_CONFIG;
    
    JsonObject params = doc.createNestedObject("params");
    params["pump_id"] = pumpId;
    params["activation_time"] = activationTime;
    params["cooldown_time"] = cooldownTime;
    
    doc["timestamp"] = millis();
    
    String commandJSON;
    serializeJson(doc, commandJSON);
    
    // Crear topic de configuración
    char topic[50];
    sprintf(topic, "motete/osmo/%s/config", deviceConfig.unitId);
    
    Serial.printf("📤 Enviando configuración para bomba %d: activación=%dms, cooldown=%dms\n", 
                  pumpId, activationTime, cooldownTime);
    
    // Enviar comando (usar QoS 0 para evitar problemas de buffer)
    if (networkManager->publishWithQoS(topic, commandJSON.c_str(), 0)) {
        Serial.printf("✅ Comando de configuración enviado para bomba %d\n", pumpId);
    } else {
        Serial.printf("❌ Error enviando configuración para bomba %d\n", pumpId);
    }
}

bool PumpController::isInitialConfigSent() const {
    return initialConfigSent;
}

void PumpController::resetInitialConfig() {
    initialConfigSent = false;
    Serial.println("🔄 Configuración inicial MQTT reiniciada");
}
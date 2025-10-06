#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include "config.h"
#include <Arduino.h>
#include "network_manager.h"
class PumpController {
private:
    int* pumpPins;        
    bool* pumpStates;     
    int* pumpLevels;      
    int* pumpActivationTimes;  // Tiempo de activación por bomba
    int* pumpCooldownTimes;    // Tiempo de cooldown por bomba
    unsigned long* pumpLastActivation; // Última activación por bomba
    int pumpCount;  // Número real de bombas
    NetworkManager* networkManager;  // Para enviar comandos MQTT
    bool initialConfigSent;  // Flag para configuración inicial
    
    // Método privado para enviar comandos de configuración
    void sendPumpConfigCommand(int pumpId, int activationTime, int cooldownTime);
    
public:
    PumpController(int count, NetworkManager* netMgr = nullptr);  // Constructor con NetworkManager opcional
    ~PumpController();
    void initialize();
    void performInitialMQTTConfig();  // Configuración inicial vía MQTT
    void setPumpState(int pumpId, bool state);
    bool getPumpState(int pumpId);
    void setPumpLevel(int pumpId, int level);
    int getPumpLevel(int pumpId);
    int getPumpCount() const { return pumpCount; }
    void updatePumps();
    bool isPumpAvailable(int pumpId);
    
    // Métodos para configuración de bombas
    void setPumpConfig(int pumpId, int activationTime, int cooldownTime);
    int getPumpActivationTime(int pumpId);
    int getPumpCooldownTime(int pumpId);
    unsigned long getPumpLastActivation(int pumpId);
    int getPumpCooldownRemaining(int pumpId);
    void resetPumpConfig(int pumpId);
    void resetAllPumpConfigs();
    bool isInitialConfigSent() const;
    void resetInitialConfig();
};

#endif
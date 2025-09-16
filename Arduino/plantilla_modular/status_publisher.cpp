#include "status_publisher.h"
#include "pump_controller.h"
#include "command_definition.h"

StatusPublisher::StatusPublisher(PumpController* pumpCtrl, NetworkManager* netMgr) 
    : pumpController(pumpCtrl), networkManager(netMgr) {}
    
String StatusPublisher::createStatusJSON() {
    // Crear array de datos de bombas
    PumpStatusData pumpData[deviceConfig.pumpCount];
    
    // Recopilar datos de cada bomba
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        pumpData[i].active = pumpController->getPumpState(i);
        pumpData[i].available = pumpController->isPumpAvailable(i);
        pumpData[i].cooldown_remaining = pumpController->getPumpCooldownRemaining(i);
        pumpData[i].level = pumpController->getPumpLevel(i);
        
        // Debug: mostrar estado de cada bomba
        Serial.print("🔍 Bomba ");
        Serial.print(i);
        Serial.print(": active=");
        Serial.print(pumpData[i].active ? "true" : "false");
        Serial.print(", available=");
        Serial.print(pumpData[i].available ? "true" : "false");
        
    }
    
    // Crear estructura de datos del dispositivo
    DeviceStatusData statusData;
    statusData.unitId = deviceConfig.unitId;
    statusData.status = "ready";
    statusData.pumps = pumpData;
    statusData.pumpCount = deviceConfig.pumpCount;
    statusData.timestamp = millis();
    
    // Usar la función de utilidad de command_definition
    return ::createStatusJSON(statusData);
}

void StatusPublisher::publishStatus() {
    String statusJSON = createStatusJSON();
    char topic[50];
    sprintf(topic, "motete/osmo/%s/status", deviceConfig.unitId);
    
    // Publicar con QoS 1 para garantizar entrega
    if (networkManager->publishWithQoS(topic, statusJSON.c_str(), 1)) {
        Serial.println("✅ Estado publicado correctamente");
    } else {
        Serial.println("❌ Error al publicar estado");
    }
}
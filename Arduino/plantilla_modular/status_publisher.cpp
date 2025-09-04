#include "status_publisher.h"

StatusPublisher::StatusPublisher(PumpController* pumpCtrl, NetworkManager* netMgr) 
    : pumpController(pumpCtrl), networkManager(netMgr) {}
    
String StatusPublisher::createStatusJSON() {
    // JSON con información completa de bombas
    StaticJsonDocument<300> doc;
    
    doc["unit_id"] = deviceConfig.unitId;
    doc["status"] = "ready";
    
    // Incluir todas las bombas con su estado
    JsonObject pumps = doc.createNestedObject("pumps");
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        JsonObject pump = pumps.createNestedObject(String(i));
        bool pumpState = pumpController->getPumpState(i);
        pump["active"] = pumpState;
        pump["available"] = pumpController->isPumpAvailable(i);
        
        // Debug: mostrar estado de cada bomba
        Serial.print("🔍 Bomba ");
        Serial.print(i);
        Serial.print(": active=");
        Serial.print(pumpState ? "true" : "false");
        Serial.print(", available=");
        Serial.println(pumpController->isPumpAvailable(i) ? "true" : "false");
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
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
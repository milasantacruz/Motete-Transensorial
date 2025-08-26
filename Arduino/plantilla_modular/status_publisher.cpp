#include "status_publisher.h"

StatusPublisher::StatusPublisher(PumpController* pumpCtrl, NetworkManager* netMgr) 
    : pumpController(pumpCtrl), networkManager(netMgr) {}
    
String StatusPublisher::createStatusJSON() {
    StaticJsonDocument<512> doc;
    
    doc["timestamp"] = "2025-01-14T10:30:01.000Z"; // TODO: Implementar timestamp real
    doc["unit_id"] = deviceConfig.unitId;
    doc["status"] = "ready";
    
    JsonObject pumps = doc.createNestedObject("pumps");
    for (int i = 1; i <= deviceConfig.pumpCount; i++) {
        JsonObject pump = pumps.createNestedObject(String(i));
        pump["active"] = pumpController->getPumpState(i-1);
        pump["level"] = pumpController->getPumpLevel(i-1);
    }
    
    doc["battery"] = random(0, 100); // TODO: Implementar lectura real de batería
    
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
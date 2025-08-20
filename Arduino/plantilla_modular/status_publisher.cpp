#include "status_publisher.h"

StatusPublisher::StatusPublisher(PumpController* pumpCtrl) : pumpController(pumpCtrl) {}

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
    Serial.printf("Longitud JSON = %u bytes\n", statusJSON.length());
    
    char topic[50];
    sprintf(topic, "motete/osmo/%s/status", deviceConfig.unitId);
    
    // La publicación se hará desde el NetworkManager
    // Esta función solo prepara el JSON
}
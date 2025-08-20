#include "main_controller.h"

MainController::MainController() 
    : statusPublisher(&pumpController), lastStatusPublish(0) {
}

void MainController::initialize() {
    Serial.begin(115200);
    
    // Configurar LED indicador
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    
    // Inicializar componentes
    pumpController.initialize();
    networkManager.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->messageCallback(topic, payload, length);
    });
    
    // Conectar a la red
    networkManager.connect();
}

void MainController::messageCallback(char* topic, byte* payload, unsigned int length) {
    // Indicador LED
    digitalWrite(2, LOW);
    delay(100);
    digitalWrite(2, HIGH);
    
    Serial.print("Mensaje recibido en: ");
    Serial.println(topic);
    
    String message;
    for (uint16_t i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    handleCommand(topic, message.c_str());
}

void MainController::handleCommand(const char* topic, const char* message) {
    // Aquí procesas los comandos recibidos
    // Por ejemplo: activar/desactivar bombas, cambiar niveles, etc.
    Serial.print("Comando recibido: ");
    Serial.println(message);
    
    // TODO: Implementar parsing de comandos JSON
}

void MainController::publishStatus() {
    String statusJSON = statusPublisher.createStatusJSON();
    char topic[50];
    sprintf(topic, "motete/osmo/%s/status", deviceConfig.unitId);
    
    if (networkManager.publish(topic, statusJSON.c_str())) {
        Serial.println("Estado publicado correctamente.");
    } else {
        Serial.println("Error al publicar estado.");
    }
}

void MainController::loop() {
    networkManager.loop();
    
    // Publicar estado periódicamente
    if (millis() - lastStatusPublish > deviceConfig.statusInterval) {
        publishStatus();
        lastStatusPublish = millis();
    }
}
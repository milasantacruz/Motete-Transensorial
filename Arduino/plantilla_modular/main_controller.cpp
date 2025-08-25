#include "main_controller.h"
#include <Arduino.h>

// Inicializar la variable estática
MainController* MainController::instancia = nullptr;

MainController::MainController() 
    : statusPublisher(&pumpController), lastStatusPublish(0) {
    // Guardar referencia a esta instancia
    instancia = this;
}

void MainController::initialize() {
    Serial.begin(115200);
    
    // Configurar LED indicador
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    
    // Inicializar componentes
    pumpController.initialize();
    networkManager.setCallback(messageCallback);
    
    // Conectar a la red
    networkManager.connect();
}

// Método estático que redirige la llamada
void MainController::messageCallback(char* topic, uint8_t* payload, unsigned int length) {
    // Verificar que existe una instancia
    if (instancia) {
        // Redirigir a la instancia real
        instancia->procesarMensaje(topic, payload, length);
    } else {
        Serial.println("ERROR: No hay instancia de MainController");
    }
}

// Método de instancia que procesa el mensaje
void MainController::procesarMensaje(char* topic, uint8_t* payload, unsigned int length) {
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
    
    // Ahora SÍ puede acceder a métodos de instancia
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
#include "main_controller.h"
#include "config.h"

MainController::MainController() : lastStatusPublish(0) {
}

MainController::~MainController() {
}

void MainController::initialize() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Motete Transensorial - Piano Server ===");
    Serial.printf("Dispositivo: %s\n", deviceConfig.unitId);
    Serial.printf("Bombas: %d\n", deviceConfig.pumpCount);
    Serial.printf("Puerto servidor: %d\n", webServerConfig.port);
    
    // Inicializar controlador de bombas
    pumpController.initialize();
    
    // Inicializar red y servidor web
    if (!networkManager.initialize(&pumpController)) {
        Serial.println("Error: No se pudo inicializar el servidor web");
        return;
    }
    
    Serial.println("Sistema inicializado correctamente");
}

void MainController::loop() {
    // Manejar red y servidor web
    networkManager.loop();
    
    // Manejar bombas
    pumpController.loop();
    
    // Publicar estado periódicamente (cada 2 segundos)
    if (millis() - lastStatusPublish > deviceConfig.statusInterval) {
        // El estado se publica automáticamente por el NetworkManager
        lastStatusPublish = millis();
    }
}

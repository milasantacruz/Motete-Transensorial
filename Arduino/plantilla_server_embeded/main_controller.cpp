#include "main_controller.h"
#include "network_manager.h"
#include "pump_controller.h"
#include "config.h"

// Constructor: NO crear objetos aquí, solo inicializar punteros a nullptr
MainController::MainController() 
    : networkManager(nullptr), pumpController(nullptr), lastStatusPublish(0) {
    // No hacer nada más aquí - evitar usar deviceConfig o Serial antes de initialize()
}

MainController::~MainController() {
    // Liberar memoria de instancias dinámicas
    if (pumpController) {
        delete pumpController;
    }
    if (networkManager) {
        delete networkManager;
    }
}

void MainController::initialize() {
    // PRIMERO: Inicializar Serial
    Serial.begin(115200);
    delay(2000);  // Dar tiempo para abrir Serial Monitor
    
    Serial.println();
    Serial.println("=== Motete Transensorial - Piano Server ===");
    Serial.println("Iniciando sistema...");
    
    // Mostrar configuración
    Serial.printf("Dispositivo: %s\n", deviceConfig.unitId);
    Serial.printf("Bombas: %d\n", deviceConfig.pumpCount);
    Serial.printf("Puerto servidor: %d\n", webServerConfig.port);
    
    // DESPUÉS de Serial.begin(): Crear objetos dinámicamente
    Serial.println("Creando PumpController...");
    pumpController = new PumpController();
    
    Serial.println("Creando NetworkManager...");
    networkManager = new NetworkManager();
    
    // Inicializar controlador de bombas
    Serial.println("Inicializando PumpController...");
    pumpController->initialize();
    
    // Inicializar red y servidor web
    Serial.println("Inicializando NetworkManager...");
    if (!networkManager->initialize(pumpController)) {
        Serial.println("Error: No se pudo inicializar el servidor web");
        return;
    }
    
    Serial.println("Sistema inicializado correctamente");
}

void MainController::loop() {
    // Verificar que los objetos existan
    if (!networkManager || !pumpController) {
        return;
    }
    
    // Manejar red y servidor web
    networkManager->loop();
    
    // Manejar bombas
    pumpController->loop();
    
    // Publicar estado periódicamente (cada 2 segundos)
    if (millis() - lastStatusPublish > deviceConfig.statusInterval) {
        // El estado se publica automáticamente por el NetworkManager
        lastStatusPublish = millis();
    }
}

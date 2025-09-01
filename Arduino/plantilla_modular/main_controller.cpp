#include "main_controller.h"
#include <Arduino.h>
#include <ArduinoJson.h>
// Inicializar la variable estática
MainController* MainController::instancia = nullptr;

// En main_controller.cpp
MainController::MainController() 
    : pumpController(deviceConfig.pumpCount),  // Pasar número de bombas
      statusPublisher(&pumpController, &networkManager), 
      lastStatusPublish(0) {
        Serial.println("🔧 Constructor MainController iniciado");  // ← LOG EN CONSTRUCTOR
        instancia = this;
        Serial.println("✅ Constructor MainController completado");
}

void MainController::initialize() {
    Serial.begin(115200);
    delay(8000);
    Serial.println("🚀 Iniciando sistema...");
    Serial.println("📌 Paso 1: Configurando LED...");
    // Configurar LED indicador
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    Serial.println("✅ LED configurado");
    Serial.println("📌 Paso 2: Inicializando bombas...");
    // Inicializar componentes
    pumpController.initialize();
    Serial.println("✅ Bombas inicializadas");
    Serial.println("📌 Paso 3: Configurando callback MQTT...");
    networkManager.setCallback(messageCallback);
    Serial.println("✅ Callback MQTT configurado");
    Serial.println("✅ Sistema iniciado completamente"); 
    // Conectar a la red
   // networkManager.connect();
}

MainController::~MainController() {
    // Destructor automático
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
    delay(500);
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

//TODO: terminar de implementar Activa/Desactivar
//TODO: agregar cambiar niveles, obtener estado, reiniciar
void MainController::processCommand(const MQTTCommand& cmd) {
    Serial.print("🔧 Procesando comando: ");
    Serial.println(cmd.action);
    
    if (cmd.action == "activate_pump") {
        int pumpId = cmd.params.toInt();
        if (pumpId >= 0 && pumpId < pumpController.getPumpCount()) {  // ✅ CORRECTO
            pumpController.setPumpState(pumpId, true);
            Serial.print("✅ Bomba ");
            Serial.print(pumpId);
            Serial.println(" activada");
        }
    }
    else if (cmd.action == "deactivate_pump") {
        int pumpId = cmd.params.toInt();
        if (pumpId >= 0 && pumpId < pumpController.getPumpCount()) {  // ✅ CORRECTO
            pumpController.setPumpState(pumpId, false);
            Serial.print("✅ Bomba ");
            Serial.print(pumpId);
            Serial.println(" desactivada");
        }
    }
    else {
        Serial.print("❌ Comando desconocido: ");
        Serial.println(cmd.action);
    }
}

void MainController::handleCommand(const char* topic, const char* message) {
    Serial.print("📩 Comando recibido en ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
    
    // Parsear JSON del comando
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.print("❌ Error parseando JSON: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Extraer datos del comando
    MQTTCommand cmd;
    cmd.commandId = doc["command_id"].as<String>();
    cmd.action = doc["action"].as<String>();
    cmd.params = doc["params"].as<String>();
    cmd.timestamp = doc["timestamp"].as<unsigned long>();
    
    // Procesar comando
    processCommand(cmd);
}

void MainController::publishStatus() {
    String statusJSON = statusPublisher.createStatusJSON();
    char topic[50];
    sprintf(topic, "motete/osmo/%s/status", deviceConfig.unitId);
    
    if (networkManager.publishWithQoS(topic, statusJSON.c_str(), 1)) {
        Serial.println("✅ Estado publicado correctamente (QoS 1)");
    } else {
        Serial.println("❌ Error al publicar estado");
    }
}

void MainController::loop() {
    static bool firstConnection = true;  // ← AGREGAR ESTA LÍNEA
    
    if (firstConnection) {
        Serial.println(" Intentando primera conexión...");
        if (networkManager.connect()) {
            Serial.println("✅ Primera conexión exitosa");
        } else {
            Serial.println("❌ Primera conexión fallida - reintentando en loop");
        }
        firstConnection = false;
    }
    
    networkManager.loop();
    
    // Publicar estado periódicamente
    if (millis() - lastStatusPublish > deviceConfig.statusInterval) {
        publishStatus();
        lastStatusPublish = millis();
    }

    delay(500);  // ← CAMBIAR a 100ms (más responsivo)
    yield();  
}
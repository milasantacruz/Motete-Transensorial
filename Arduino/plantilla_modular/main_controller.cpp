#include "main_controller.h"
#include "pump_controller.h"
#include "status_publisher.h"
#include <Arduino.h>
#include <ArduinoJson.h>
// Inicializar la variable estática
MainController* MainController::instancia = nullptr;

// En main_controller.cpp
MainController::MainController() 
    : lastStatusPublish(0) {
        Serial.println("🔧 Constructor MainController iniciado");  // ← LOG EN CONSTRUCTOR
        
        // Crear instancias dinámicamente
        pumpController = new PumpController(deviceConfig.pumpCount, &networkManager);
        statusPublisher = new StatusPublisher(pumpController, &networkManager);
        
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
    pumpController->initialize();
    Serial.println("✅ Bombas inicializadas");
    Serial.println("📌 Paso 3: Configurando callback MQTT...");
    networkManager.setCallback(messageCallback);
    Serial.println("✅ Callback MQTT configurado");
    Serial.println("✅ Sistema iniciado completamente"); 
    
    
}

MainController::~MainController() {
    // Liberar memoria de instancias dinámicas
    delete pumpController;
    delete statusPublisher;
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

void MainController::processCommand(const MQTTCommand& cmd) {
    Serial.print("🔧 Procesando comando: ");
    Serial.println(cmd.action);
    
    // Validar parámetros primero
    if (!validateCommandParams(cmd)) {
        Serial.print("❌ Parámetros inválidos para comando: ");
        Serial.println(cmd.action);
        CommandResponse errorResponse = createResponse(ResponseCodes::INVALID_PARAMS, ErrorMessages::INVALID_PARAMS, cmd.commandId);
        sendCommandResponse(errorResponse);
        return;
    }
    
    // Procesar comando según la acción
    if (cmd.action == Commands::ACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        Serial.print("🔧 Activando bomba ");
        Serial.print(params.pumpId);
        Serial.print(" por ");
        Serial.print(params.duration);
        Serial.println(" ms");
        
        // Verificar si la bomba está disponible
        if (!pumpController->isPumpAvailable(params.pumpId) && !params.force) {
            Serial.print("❌ Bomba ");
            Serial.print(params.pumpId);
            Serial.println(" en cooldown");
            CommandResponse errorResponse = createResponse(ResponseCodes::PUMP_BUSY, ErrorMessages::PUMP_BUSY, cmd.commandId);
            sendCommandResponse(errorResponse);
            return;
        }
        
        // Activar bomba
        pumpController->setPumpState(params.pumpId, true);
        Serial.print("✅ Bomba ");
        Serial.print(params.pumpId);
        Serial.println(" activada");
        
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::PUMP_ACTIVATED, cmd.commandId);
        sendCommandResponse(successResponse);
    }
    else if (cmd.action == Commands::DEACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        Serial.print("🔧 Desactivando bomba ");
        Serial.println(params.pumpId);
        
        // Desactivar bomba
        pumpController->setPumpState(params.pumpId, false);
        Serial.print("✅ Bomba ");
        Serial.print(params.pumpId);
        Serial.println(" desactivada");
        
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::PUMP_DEACTIVATED, cmd.commandId);
        sendCommandResponse(successResponse);
    }
    else if (cmd.action == Commands::GET_STATUS) {
        Serial.println("🔧 Obteniendo estado del dispositivo");
        
        // Publicar estado actual
        publishStatus();
        
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::STATUS_RETRIEVED, cmd.commandId);
        sendCommandResponse(successResponse);
    }
    else if (cmd.action == Commands::SET_PUMP_CONFIG) {
        PumpConfigParams params = extractPumpConfigParams(cmd.params);
        Serial.print("🔧 Configurando bomba ");
        Serial.print(params.pumpId);
        Serial.print(" - Activación: ");
        Serial.print(params.activationTime);
        Serial.print(" ms, Cooldown: ");
        Serial.print(params.cooldownTime);
        Serial.println(" ms");
        
        // Integrar configuración de parámetros de bomba
        pumpController->setPumpConfig(params.pumpId, params.activationTime, params.cooldownTime);
        
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::CONFIG_UPDATED, cmd.commandId);
        sendCommandResponse(successResponse);
    }
    else if (cmd.action == Commands::REBOOT) {
        Serial.println("🔧 Reiniciando dispositivo...");
        
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::REBOOT_INITIATED, cmd.commandId);
        sendCommandResponse(successResponse);
        
        delay(1000);
        ESP.restart();
    }
    else if (cmd.action == Commands::RESET_CONFIG) {
        Serial.println("🔧 Restableciendo configuración...");
        
        // Implementar reset de configuración
        resetDeviceConfig();
        
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::CONFIG_RESET, cmd.commandId);
        sendCommandResponse(successResponse);
    }
    else {
        Serial.print("❌ Comando no reconocido: ");
        Serial.println(cmd.action);
        CommandResponse errorResponse = createResponse(ResponseCodes::INVALID_COMMAND, ErrorMessages::INVALID_COMMAND, cmd.commandId);
        sendCommandResponse(errorResponse);
    }
}

void MainController::handleCommand(const char* topic, const char* message) {
    Serial.print("📩 Comando recibido en ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
    
    // Parsear comando usando la función de command_definition
    MQTTCommand cmd = parseCommandFromJSON(String(message));
    
    if (cmd.action == "") {
        Serial.println("❌ Error parseando comando JSON");
        CommandResponse errorResponse = createResponse(ResponseCodes::INVALID_PARAMS, ErrorMessages::INVALID_PARAMS);
        sendCommandResponse(errorResponse);
        return;
    }
    
    // Procesar comando
    processCommand(cmd);
}

void MainController::publishStatus() {
    Serial.println("📊 Publicando estado...");
    
    // Verificar conexión MQTT antes de publicar
    if (!networkManager.isMQTTConnected()) {
        Serial.println("❌ MQTT desconectado, no se puede publicar estado");
        return;
    }
    
    // Usar el método del StatusPublisher que ya maneja todo
    statusPublisher->publishStatus();
}

void MainController::sendCommandResponse(const CommandResponse& response) {
    String responseJSON = createResponseJSON(response);
    char topic[50];
    sprintf(topic, "motete/osmo/%s/response", deviceConfig.unitId);
    
    Serial.print("📤 Intentando enviar respuesta: ");
    Serial.println(response.message);
    
    // Probar con método simple primero
    if (networkManager.publish(topic, responseJSON.c_str())) {
        Serial.print("✅ Respuesta enviada (método simple): ");
        Serial.println(response.message);
    } else {
        Serial.println("❌ Error con método simple, intentando QoS 0...");
        if (networkManager.publishWithQoS(topic, responseJSON.c_str(), 0)) {
            Serial.print("✅ Respuesta enviada (QoS 0): ");
            Serial.println(response.message);
        } else {
            Serial.println("❌ Error al enviar respuesta con todos los métodos");
        }
    }
}

void MainController::loop() {
    static bool firstConnection = true;  
    
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
    
    // Configuración inicial de bombas (una sola vez después de conectar)
    if (networkManager.isMQTTConnected() && !pumpController->isInitialConfigSent()) {
        pumpController->performInitialMQTTConfig();
    }
    
    // Actualizar estado de bombas (cooldown, desactivación automática)
    pumpController->updatePumps();
    
    // Publicar estado periódicamente
    if (millis() - lastStatusPublish > deviceConfig.statusInterval) {
        publishStatus();
        lastStatusPublish = millis();
    }

    delay(100);  
    yield();  
}

void MainController::resetDeviceConfig() {
    Serial.println("🔄 Restableciendo configuración del dispositivo...");
    
        // Reset todas las configuraciones de bombas
    pumpController->resetAllPumpConfigs();
    
    // Reset configuración inicial para que se vuelva a ejecutar
    pumpController->resetInitialConfig();
    
    // Reset otras configuraciones si es necesario
    // Por ejemplo: WiFi, MQTT, etc.
    
    Serial.println("✅ Configuración restablecida correctamente");
}
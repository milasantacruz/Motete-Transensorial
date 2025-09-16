#include "command_definition.h"
#include <ArduinoJson.h>
#include "config.h"

// Definición de acciones disponibles
namespace Commands {
    const char* ACTIVATE_PUMP = "activate_pump";
    const char* DEACTIVATE_PUMP = "deactivate_pump";
    const char* GET_STATUS = "get_status";
    const char* SET_PUMP_CONFIG = "set_pump_config";
    const char* REBOOT = "reboot";
    const char* RESET_CONFIG = "reset_config";
    const char* HEARTBEAT = "heartbeat";
}

// Códigos de respuesta
namespace ResponseCodes {
    const int SUCCESS = 200;
    const int INVALID_COMMAND = 400;
    const int INVALID_PARAMS = 422;
    const int PUMP_BUSY = 423;
    const int PUMP_NOT_FOUND = 404;
    const int INTERNAL_ERROR = 500;
    const int NETWORK_ERROR = 503;
}

// Constantes de validación
namespace Validation {
    const int MIN_PUMP_ID = 0;
    const int MAX_PUMP_ID = 3;
    const int MIN_DURATION = 100;      // 100ms mínimo
    const int MAX_DURATION = 60000;    // 60 segundos máximo
    const int MIN_CONFIG_TIME = 1000;  // 1 segundo mínimo
    const int MAX_CONFIG_TIME = 300000; // 5 minutos máximo
}

// Mensajes de error predefinidos
namespace ErrorMessages {
    const char* INVALID_COMMAND = "Comando no reconocido";
    const char* INVALID_PUMP_ID = "ID de bomba inválido";
    const char* INVALID_DURATION = "Duración inválida";
    const char* PUMP_BUSY = "Bomba en cooldown";
    const char* PUMP_NOT_FOUND = "Bomba no encontrada";
    const char* INVALID_PARAMS = "Parámetros inválidos";
    const char* NETWORK_ERROR = "Error de red";
    const char* INTERNAL_ERROR = "Error interno del sistema";
}

// Mensajes de éxito predefinidos
namespace SuccessMessages {
    const char* PUMP_ACTIVATED = "Bomba activada correctamente";
    const char* PUMP_DEACTIVATED = "Bomba desactivada correctamente";
    const char* CONFIG_UPDATED = "Configuración actualizada";
    const char* STATUS_RETRIEVED = "Estado obtenido correctamente";
    const char* REBOOT_INITIATED = "Reinicio iniciado";
    const char* CONFIG_RESET = "Configuración restablecida";
}

// Función para crear respuesta de comando
CommandResponse createResponse(int code, const String& message, const String& commandId) {
    CommandResponse response;
    response.code = code;
    response.message = message;
    response.commandId = commandId;
    response.success = (code >= 200 && code < 300);
    response.timestamp = millis();
    return response;
}

// Función para validar ID de bomba
bool isValidPumpId(int pumpId) {
    return pumpId >= Validation::MIN_PUMP_ID && pumpId <= Validation::MAX_PUMP_ID;
}

// Función para validar duración
bool isValidDuration(int duration) {
    return duration >= Validation::MIN_DURATION && duration <= Validation::MAX_DURATION;
}

// Función para validar tiempo de configuración
bool isValidConfigTime(int time) {
    return time >= Validation::MIN_CONFIG_TIME && time <= Validation::MAX_CONFIG_TIME;
}

// Función para extraer parámetros de activación de bomba
PumpActivationParams extractPumpActivationParams(const String& params) {
    PumpActivationParams pumpParams;
    pumpParams.pumpId = -1;
    pumpParams.duration = 10000; // Default 10 segundos
    pumpParams.force = false;
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, params);
    
    if (!error) {
        if (doc.containsKey("pump_id")) {
            pumpParams.pumpId = doc["pump_id"];
        }
        if (doc.containsKey("duration")) {
            pumpParams.duration = doc["duration"];
        }
        if (doc.containsKey("force")) {
            pumpParams.force = doc["force"];
        }
    }
    
    return pumpParams;
}

// Función para extraer parámetros de configuración de bomba
PumpConfigParams extractPumpConfigParams(const String& params) {
    PumpConfigParams configParams;
    configParams.pumpId = -1;
    configParams.activationTime = 10000; // Default 10 segundos
    configParams.cooldownTime = 30000;   // Default 30 segundos
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, params);
    
    if (!error) {
        if (doc.containsKey("pump_id")) {
            configParams.pumpId = doc["pump_id"];
        }
        if (doc.containsKey("activation_time")) {
            configParams.activationTime = doc["activation_time"];
        }
        if (doc.containsKey("cooldown_time")) {
            configParams.cooldownTime = doc["cooldown_time"];
        }
    }
    
    return configParams;
}

// Función para validar parámetros de comando
bool validateCommandParams(const MQTTCommand& cmd) {
    if (cmd.action == Commands::ACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        if (!isValidPumpId(params.pumpId)) {
            return false;
        }
        if (!isValidDuration(params.duration)) {
            return false;
        }
        return true;
    }
    else if (cmd.action == Commands::DEACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        return isValidPumpId(params.pumpId);
    }
    else if (cmd.action == Commands::SET_PUMP_CONFIG) {
        PumpConfigParams params = extractPumpConfigParams(cmd.params);
        if (!isValidPumpId(params.pumpId)) {
            return false;
        }
        if (!isValidConfigTime(params.activationTime)) {
            return false;
        }
        if (!isValidConfigTime(params.cooldownTime)) {
            return false;
        }
        return true;
    }
    else if (cmd.action == Commands::GET_STATUS) {
        return true; // No requiere parámetros
    }
    else if (cmd.action == Commands::REBOOT || cmd.action == Commands::RESET_CONFIG) {
        return true; // No requiere parámetros
    }
    
    return false; // Comando no reconocido
}

// Función para crear JSON de respuesta
String createResponseJSON(const CommandResponse& response) {
    StaticJsonDocument<256> doc;
    doc["code"] = response.code;
    doc["message"] = response.message;
    doc["command_id"] = response.commandId;
    doc["success"] = response.success;
    doc["timestamp"] = response.timestamp;
    doc["unit_id"] = deviceConfig.unitId;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// Función para crear JSON de estado
String createStatusJSON(const DeviceStatusData& statusData) {
    StaticJsonDocument<512> doc;
    doc["unit_id"] = statusData.unitId;
    doc["status"] = statusData.status;
   // doc["timestamp"] = statusData.timestamp;
   // doc["pump_count"] = statusData.pumpCount;
    
    // Crear objeto de bombas con datos reales
    JsonObject pumps = doc.createNestedObject("pumps");
    for (int i = 0; i < statusData.pumpCount; i++) {
        JsonObject pump = pumps.createNestedObject(String(i));
        pump["active"] = statusData.pumps[i].active;
        pump["available"] = statusData.pumps[i].available;
       // pump["cooldown_remaining"] = statusData.pumps[i].cooldown_remaining;
       // pump["level"] = statusData.pumps[i].level;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// Función para crear JSON de error
String createErrorJSON(const String& errorType, const String& message) {
    StaticJsonDocument<256> doc;
    doc["error_type"] = errorType;
    doc["message"] = message;
    doc["timestamp"] = millis();
    doc["unit_id"] = deviceConfig.unitId;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// Función para parsear comando desde JSON
MQTTCommand parseCommandFromJSON(const String& json) {
    MQTTCommand cmd;
    cmd.commandId = "";
    cmd.action = "";
    cmd.params = "";
    cmd.timestamp = 0;
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (!error) {
        if (doc.containsKey("command_id")) {
            cmd.commandId = doc["command_id"].as<String>();
        }
        if (doc.containsKey("action")) {
            cmd.action = doc["action"].as<String>();
        }
        if (doc.containsKey("params")) {
            // Convertir params a string JSON
            String paramsStr;
            serializeJson(doc["params"], paramsStr);
            cmd.params = paramsStr;
        }
        if (doc.containsKey("timestamp")) {
            cmd.timestamp = doc["timestamp"];
        }
    }
    
    return cmd;
}

// Función para procesar comandos (implementación básica)
bool processCommand(const MQTTCommand& cmd) {
    // Validar parámetros primero
    if (!validateCommandParams(cmd)) {
        Serial.print("❌ Parámetros inválidos para comando: ");
        Serial.println(cmd.action);
        return false;
    }
    
    // Procesar comando según la acción
    if (cmd.action == Commands::ACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        Serial.print("🔧 Activando bomba ");
        Serial.print(params.pumpId);
        Serial.print(" por ");
        Serial.print(params.duration);
        Serial.println(" ms");
        
        // Aquí se integrará con PumpController
        // pumpController.activatePump(params.pumpId, params.duration, params.force);
        
        return true;
    }
    else if (cmd.action == Commands::DEACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        Serial.print("🔧 Desactivando bomba ");
        Serial.println(params.pumpId);
        
        // Aquí se integrará con PumpController
        // pumpController.deactivatePump(params.pumpId);
        
        return true;
    }
    else if (cmd.action == Commands::GET_STATUS) {
        Serial.println("🔧 Obteniendo estado del dispositivo");
        
        // Aquí se integrará con StatusPublisher
        // String status = statusPublisher.createStatusJSON();
        // networkManager.publishWithQoS(statusTopic, status.c_str(), 1);
        
        return true;
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
        
        // Integrar con PumpController para actualizar configuración
        // Nota: Se necesita acceso a la instancia de PumpController
        // pumpController.setPumpConfig(params.pumpId, params.activationTime, params.cooldownTime);
        
        return true;
    }
    else if (cmd.action == Commands::REBOOT) {
        Serial.println("🔧 Reiniciando dispositivo...");
        delay(1000);
        ESP.restart();
        return true;
    }
    else if (cmd.action == Commands::RESET_CONFIG) {
        Serial.println("🔧 Restableciendo configuración...");
        
        // Implementar reset de configuración
        // Nota: Se necesita acceso a la instancia de PumpController
        // pumpController.resetAllPumpConfigs();
        
        return true;
    }
    else {
        Serial.print("❌ Comando no reconocido: ");
        Serial.println(cmd.action);
        return false;
    }
}

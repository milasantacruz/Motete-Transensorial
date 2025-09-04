#ifndef COMMAND_DEFINITIONS_H
#define COMMAND_DEFINITIONS_H

#include <Arduino.h>

// Estructura para comandos MQTT
struct MQTTCommand {
    String commandId;
    String action;
    String params;
    unsigned long timestamp;
};

// Definición de acciones disponibles
namespace Commands {
    extern const char* ACTIVATE_PUMP;
    extern const char* DEACTIVATE_PUMP;
    extern const char* GET_STATUS;
    extern const char* SET_PUMP_CONFIG;
    extern const char* REBOOT;
    extern const char* RESET_CONFIG;
    extern const char* HEARTBEAT;
}

// Estructura para parámetros de activación de bomba
struct PumpActivationParams {
    int pumpId;
    int duration;  // en milisegundos
    bool force;    // forzar activación aunque esté en cooldown
};

// Estructura para parámetros de configuración de bomba
struct PumpConfigParams {
    int pumpId;
    int activationTime;
    int cooldownTime;
};

// Códigos de respuesta
namespace ResponseCodes {
    extern const int SUCCESS;
    extern const int INVALID_COMMAND;
    extern const int INVALID_PARAMS;
    extern const int PUMP_BUSY;
    extern const int PUMP_NOT_FOUND;
    extern const int INTERNAL_ERROR;
    extern const int NETWORK_ERROR;
}

// Estructura para respuestas de comandos
struct CommandResponse {
    int code;
    String message;
    String commandId;
    bool success;
    unsigned long timestamp;
};

// Función para crear respuesta de comando
CommandResponse createResponse(int code, const String& message, const String& commandId = "");

// Función para procesar comandos
bool processCommand(const MQTTCommand& cmd);

// Función para validar parámetros de comando
bool validateCommandParams(const MQTTCommand& cmd);

// Función para extraer parámetros de activación de bomba
PumpActivationParams extractPumpActivationParams(const String& params);

// Función para extraer parámetros de configuración de bomba
PumpConfigParams extractPumpConfigParams(const String& params);

// Función para crear JSON de respuesta
String createResponseJSON(const CommandResponse& response);

// Función para crear JSON de estado
String createStatusJSON();

// Función para crear JSON de error
String createErrorJSON(const String& errorType, const String& message);

// Función para parsear comando desde JSON
MQTTCommand parseCommandFromJSON(const String& json);

// Función para validar ID de bomba
bool isValidPumpId(int pumpId);

// Función para validar duración
bool isValidDuration(int duration);

// Función para validar tiempo de configuración
bool isValidConfigTime(int time);

// Constantes de validación
namespace Validation {
    extern const int MIN_PUMP_ID;
    extern const int MAX_PUMP_ID;
    extern const int MIN_DURATION;
    extern const int MAX_DURATION;
    extern const int MIN_CONFIG_TIME;
    extern const int MAX_CONFIG_TIME;
}

// Mensajes de error predefinidos
namespace ErrorMessages {
    extern const char* INVALID_COMMAND;
    extern const char* INVALID_PUMP_ID;
    extern const char* INVALID_DURATION;
    extern const char* PUMP_BUSY;
    extern const char* PUMP_NOT_FOUND;
    extern const char* INVALID_PARAMS;
    extern const char* NETWORK_ERROR;
    extern const char* INTERNAL_ERROR;
}

// Mensajes de éxito predefinidos
namespace SuccessMessages {
    extern const char* PUMP_ACTIVATED;
    extern const char* PUMP_DEACTIVATED;
    extern const char* CONFIG_UPDATED;
    extern const char* STATUS_RETRIEVED;
    extern const char* REBOOT_INITIATED;
    extern const char* CONFIG_RESET;
}

#endif // COMMAND_DEFINITIONS_H

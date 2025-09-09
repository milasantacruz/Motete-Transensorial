# Documentación Arquitectura Modular OSMO

## Estructura de Archivos

### `config.h`
Define estructuras de datos para configuración WiFi, MQTT y dispositivo.

### `config.cpp` 
Instancia configuraciones con valores específicos del dispositivo OSMO.

### `network_manager.h`
Interfaz para gestión de WiFi y MQTT.

### `network_manager.cpp`
Implementa conectividad WiFi y comunicación MQTT con broker.

### `pump_controller.h`
Interfaz para control de hardware de bombas de agua.

### `pump_controller.cpp`
Implementa control físico de bombas a través de pines GPIO.

### `status_publisher.h`
Interfaz para generación y publicación de estado del dispositivo.

### `status_publisher.cpp`
Construye mensajes JSON con estado actual del sistema.

### `command_definition.h`
Define el contrato de comandos MQTT y respuestas compartido por firmware y servidor:
- Estructuras: `MQTTCommand`, `PumpActivationParams`, `PumpConfigParams`, `CommandResponse`.
- Constantes (como `Commands`, `ResponseCodes`, `Validation`, `ErrorMessages`, `SuccessMessages`) declaradas como `extern`.
- Utilidades: validación de parámetros, extracción de params, creación de JSON de respuesta/estado.

### `command_definition.cpp`
Implementa las utilidades declaradas en `command_definition.h` y define los `extern`.

### `main_controller.h`
Interfaz del controlador principal que orquesta todos los módulos.

### `main_controller.cpp`
Implementa lógica principal y coordina todos los módulos.

• Integra `command_definition` para parsear/validar comandos y generar respuestas.
• Usa `StatusPublisher` para publicar estado.

### `test_osmo_norte.ino`
Archivo principal de Arduino que instancia y ejecuta el controlador.

## Flujo de Información

### Inicialización
setup() → MainController::initialize() → PumpController + NetworkManager + publishStatus()

### Bucle Principal  
loop() → MainController::loop() → networkManager.loop() + publishStatus() periódico

### Recepción de Comandos
Mensaje MQTT → NetworkManager → MainController::messageCallback() → handleCommand() → PumpController

### Publicación de Estado
Timer → publishStatus() → createStatusJSON() → networkManager.publish()


📊 FLUJO DETALLADO DE DATOS

1. INICIALIZACIÓN
setup() → MainController::initialize() → 
├── PumpController::initialize() (configura pines GPIO)
├── NetworkManager::connect() → 
│   ├── setupWiFi() (conecta a WiFi)
│   └── setupMQTT() (conecta a broker, suscribe a comandos)//TODO: no Mqtt? --> http
└── publishStatus() (envía estado inicial via `StatusPublisher`)

2. BUCLE PRINCIPAL
loop() → MainController::loop() →
├── networkManager.loop() (mantiene conexiones MQTT)
├── Verifica intervalo de publicación
└── Si es tiempo → publishStatus() → 
    ├── StatusPublisher::createStatusJSON() (genera JSON simplificado)
    └── NetworkManager::publish / publishWithQoS() (envía por MQTT)


3. RECEPCIÓN DE COMANDOS
Mensaje MQTT → NetworkManager → MainController::messageCallback() →
├── parseCommandFromJSON() (convierte payload a `MQTTCommand`)
├── validateCommandParams() (valida acción y parámetros)
├── processCommand() (en `MainController`)
│   ├── ACTIVATE_PUMP → `PumpController::setPumpState(pumpId, true)`
│   ├── DEACTIVATE_PUMP → `PumpController::setPumpState(pumpId, false)`
│   ├── GET_STATUS → `publishStatus()`
│   ├── SET_PUMP_CONFIG → `PumpController::setPumpConfig(pumpId, activation, cooldown)`
│   ├── RESET_CONFIG → `resetDeviceConfig()` + `PumpController::resetAllPumpConfigs()`
│   └── REBOOT → `ESP.restart()`
└── createResponseJSON() → NetworkManager::publish respuesta en tópico `/response`


4. PUBLICACIÓN DE ESTADO
Timer → publishStatus() → 
├── StatusPublisher::createStatusJSON() (unit_id, status, pumps[active, available])
└── NetworkManager::publish()/publishWithQoS() (tópico `motete/osmo/{unit_id}/status`)

---

## Contrato de Comandos (command_definition)

### Tópicos MQTT
- Comandos del director → dispositivo: `motete/director/commands/{unit_id}`
- Respuestas del dispositivo: `motete/osmo/{unit_id}/response`
- Estado del dispositivo: `motete/osmo/{unit_id}/status`

### Acciones soportadas (`Commands`)
- `activate_pump`
- `deactivate_pump`
- `get_status`
- `set_pump_config`
- `reset_config`
- `reboot`

### Esquemas JSON

Solicitud genérica (`MQTTCommand`):
```
{
  "command_id": "cmd_...",    // opcional
  "action": "activate_pump" | "deactivate_pump" | ...,
  "params": { ... },
  "timestamp": 1736980000000   // opcional
}
```

Parámetros por acción:
- `activate_pump`: `{ "pump_id": Number, "duration": Number, "force": Boolean? }`
- `deactivate_pump`: `{ "pump_id": Number }`
- `set_pump_config`: `{ "pump_id": Number, "activation_time": Number, "cooldown_time": Number }`
- `get_status`, `reset_config`, `reboot`: `{}`

Respuesta (`CommandResponse`):
```
{
  "command_id": "cmd_...",
  "action": "activate_pump",
  "code": Number,              // `ResponseCodes`
  "message": String,           // texto humano
  "data": { ... }              // opcional (p.ej. estado)
}
```

Estado (`StatusPublisher::createStatusJSON`):
```
{
  "unit_id": "osmo_norte",
  "status": "ready" | "busy" | "error",
  "pumps": {
    "0": { "active": Boolean, "available": Boolean },
    "1": { "active": Boolean, "available": Boolean },
    ...
  }
}
```

### Validaciones clave (`Validation`)
- `isValidPumpId(pumpId, pumpCount)`
- `isValidDuration(ms)` y `isValidConfigTime(ms)`
- `validateCommandParams(cmd)`

### Integración con módulos
- `MainController` usa `parseCommandFromJSON`, `processCommand`, `createResponseJSON`.
- `PumpController` provee `setPumpState`, `isPumpAvailable`, `setPumpConfig`, `updatePumps` (auto-apagado por `activation_time`).
- `StatusPublisher` genera el JSON de estado simplificado para reducir tamaño.


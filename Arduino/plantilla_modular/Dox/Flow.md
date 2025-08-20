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

### `main_controller.h`
Interfaz del controlador principal que orquesta todos los módulos.

### `main_controller.cpp`
Implementa lógica principal y coordina todos los módulos.

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
└── publishStatus() (envía estado inicial)

2. BUCLE PRINCIPAL
loop() → MainController::loop() →
├── networkManager.loop() (mantiene conexiones MQTT)
├── Verifica intervalo de publicación
└── Si es tiempo → publishStatus() → 
    ├── createStatusJSON() (genera JSON)
    └── networkManager.publish() (envía por MQTT)


3. RECEPCIÓN DE COMANDOS
Mensaje MQTT → NetworkManager → MainController::messageCallback() →
├── Indicador LED (parpadeo)
├── handleCommand() (procesa comando)
└── PumpController::setPumpState() (activa/desactiva bomba)


4. PUBLICACIÓN DE ESTADO
Timer → publishStatus() → 
├── PumpController::getPumpState() (estado actual bombas)
├── PumpController::getPumpLevel() (niveles actuales)
├── createStatusJSON() (construye JSON)
└── networkManager.publish() (envía a tópico "motete/osmo/osmo_norte/status")

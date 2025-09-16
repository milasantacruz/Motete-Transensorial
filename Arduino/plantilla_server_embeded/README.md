# Motete Transensorial - Piano Server Embebido

Este proyecto es una versión simplificada de `plantilla_modular` que elimina MQTT y agrega un servidor web embebido para servir la página "piano".

## Características

- **Sin MQTT**: Comunicación directa a través de servidor web
- **Servidor web embebido**: Puerto 80 para la interfaz web
- **WebSocket**: Puerto 81 para comunicación en tiempo real
- **API REST**: Endpoints para comandos y estado
- **Control de bombas**: Con sistema de cooldown
- **Interfaz web**: Piano interactivo con p5.js

## Configuración

### 1. WiFi
Editar `config.cpp`:
```cpp
WiFiConfig wifiConfig = {
    "TuSSID",           // SSID de tu red WiFi
    "TuPassword"        // Contraseña de tu red WiFi
};
```

### 2. Dispositivo
```cpp
DeviceConfig deviceConfig = {
    "OSMO_PIANO_001",   // ID único del dispositivo
    4,                  // Número de bombas
    2000,               // Intervalo de publicación de estado (ms)
    {2, 4, 5, 16},      // Pines de las bombas (GPIO)
    {1000, 3000}        // Tiempo de activación y cooldown por defecto (ms)
};
```

### 3. Servidor web
```cpp
WebServerConfig webServerConfig = {
    80,                 // Puerto del servidor web
    "osmo-piano"        // Hostname del dispositivo
};
```

## Uso

1. **Compilar y subir** el código al ESP8266
2. **Conectar las bombas** a los pines configurados
3. **Conectar a WiFi** (verificar en Serial Monitor)
4. **Acceder a la interfaz**: `http://[IP_DEL_DISPOSITIVO]/piano.html`

## API Endpoints

### GET /api/status
Obtiene el estado del sistema y las bombas.

**Parámetros:**
- `simulate=true` (opcional): Modo simulación

**Respuesta:**
```json
{
  "server_connected": true,
  "connected_osmos": [
    {
      "unit_id": "OSMO_PIANO_001",
      "pumps": {
        "0": {"pump_id": 0, "cooldown_remaining": 0, "is_active": false},
        "1": {"pump_id": 1, "cooldown_remaining": 0, "is_active": false}
      }
    }
  ],
  "osmo_configs": {
    "OSMO_PIANO_001": {
      "pump_0": {"activationTime": 1000, "cooldownTime": 3000},
      "pump_1": {"activationTime": 1000, "cooldownTime": 3000}
    }
  },
  "cooldowns": {}
}
```

### POST /api/command/{unitId}
Envía comandos a las bombas.

**Body:**
```json
{
  "action": "activate_pump",
  "params": {"pump_id": 0}
}
```

**Respuesta:**
```json
{
  "success": true,
  "message": "Bomba activada",
  "pump_id": 0
}
```

## WebSocket

El WebSocket en puerto 81 envía actualizaciones en tiempo real:

```json
{
  "event": "status",
  "timestamp": 12345,
  "server_connected": true,
  "wifi_connected": true,
  "ip_address": "192.168.1.100"
}
```

## Estructura del Proyecto

```
plantilla_server_embeded/
├── plantilla_server_embeded.ino    # Archivo principal
├── main_controller.h/.cpp          # Controlador principal
├── network_manager.h/.cpp          # Servidor web y WiFi
├── pump_controller.h/.cpp          # Control de bombas
├── config.h/.cpp                   # Configuración
├── piano.html                      # Página web
├── piano.css                       # Estilos
├── piano.js                        # JavaScript (p5.js)
├── common.css                      # Estilos comunes
└── README.md                       # Esta documentación
```

## Diferencias con plantilla_modular

| Característica | plantilla_modular | plantilla_server_embeded |
|----------------|-------------------|--------------------------|
| MQTT | ✅ | ❌ |
| Servidor web | ❌ | ✅ |
| WebSocket | ❌ | ✅ |
| API REST | ❌ | ✅ |
| Interfaz web | ❌ | ✅ |
| Control de bombas | ✅ | ✅ |
| Configuración | ✅ | ✅ |

## Requisitos

- ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
- Librerías:
  - ESP8266WiFi
  - ESP8266WebServer
  - WebSocketsServer
  - ArduinoJson

## Troubleshooting

1. **No se conecta a WiFi**: Verificar credenciales en `config.cpp`
2. **No accede a la página**: Verificar IP en Serial Monitor
3. **Bombas no responden**: Verificar pines en `config.cpp`
4. **WebSocket no funciona**: Verificar que el puerto 81 esté libre

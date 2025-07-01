# Desarrollo de Software - Broker MQTT
## Proyecto: Motete Transensorial - Osmo v5

### 1. Arquitectura General del Broker MQTT

El broker MQTT actúa como el sistema nervioso central del "Motete transensorial", permitiendo la comunicación entre el Director (Raspberry Pi) y los tres Instrumentos (Osmos v5 con ESP32).

#### 1.1 Componentes del Sistema
```
┌─────────────────┐     MQTT      ┌─────────────────┐
│   DIRECTOR      │◄──────────────┤  OSMO v5 Norte  │
│  (Raspberry Pi) │               │    (ESP32)      │
│                 │               └─────────────────┘
│  ┌───────────┐  │
│  │MQTT Broker│  │     MQTT      ┌─────────────────┐
│  │(Mosquitto)│  │◄──────────────┤  OSMO v5 Sur    │
│  └───────────┘  │               │    (ESP32)      │
│                 │               └─────────────────┘
│  ┌───────────┐  │
│  │Node.js App│  │     MQTT      ┌─────────────────┐
│  │(Orquestador)│ │◄──────────────┤  OSMO v5 Este   │
│  └───────────┘  │               │    (ESP32)      │
│                 │               └─────────────────┘
└─────────────────┘
        │
        │ WebSockets
        ▼
┌─────────────────┐
│ Interfaz Web    │
│ (Tablets/Móviles)│
└─────────────────┘
```

### 2. Estructura de Topics MQTT

#### 2.1 Jerarquía de Topics
```
motete/
├── director/
│   ├── commands/          # Comandos del Director a los Osmos
│   ├── status/           # Estado general del sistema
│   └── broadcast/        # Mensajes para todos los Osmos
├── osmo/
│   ├── [unidad_id]/
│   │   ├── status/       # Estado individual de cada Osmo
│   │   ├── sensors/      # Datos de sensores
│   │   ├── actions/      # Confirmación de acciones
│   │   └── errors/       # Reportes de errores
│   └── discovery/        # Auto-descubrimiento de unidades
└── partitura/
    ├── sync/             # Sincronización de tiempo
    ├── events/           # Eventos de la partitura en ejecución
    └── control/          # Control de reproducción (play/pause/stop)
```

#### 2.2 Identificadores de Unidades
- `osmo_norte` - Osmo v5 posicionado al norte
- `osmo_sur` - Osmo v5 posicionado al sur  
- `osmo_este` - Osmo v5 posicionado al este

### 3. Tipos de Mensajes MQTT

#### 3.1 Comandos del Director → Osmos

**Topic**: `motete/director/commands/[unidad_id]`
**QoS**: 1 (garantiza entrega)

```json
{
  "timestamp": "2025-01-14T10:30:00.000Z",
  "command_id": "cmd_001234",
  "action": "aroma",
  "params": {
    "pump": 2,
    "duration": 3000,
    "intensity": 80
  }
}
```

**Tipos de acciones disponibles**:
- `aroma` - Activar bomba de aroma
- `blower` - Controlar soplador
- `audio` - Reproducir sonido local
- `led` - Control de iluminación
- `calibrate` - Calibración de sistemas
- `maintenance` - Modo mantenimiento

#### 3.2 Estado de Osmos → Director

**Topic**: `motete/osmo/[unidad_id]/status`
**QoS**: 0 (información de estado)

```json
{
  "timestamp": "2025-01-14T10:30:01.000Z",
  "unit_id": "osmo_norte",
  "status": "ready",
  "pumps": {
    "1": { "active": false, "level": 95 },
    "2": { "active": true, "level": 87 },
    "3": { "active": false, "level": 72 },
    "4": { "active": false, "level": 45 },
    "5": { "active": false, "level": 88 },
    "6": { "active": false, "level": 91 },
    "7": { "active": false, "level": 23 },
    "8": { "active": false, "level": 67 }
  },
  "temperature": 245,
  "wifi_strength": -45,
  "uptime": 3600000
}
```

#### 3.3 Confirmación de Acciones

**Topic**: `motete/osmo/[unidad_id]/actions`
**QoS**: 1 (confirmación importante)

```json
{
  "timestamp": "2025-01-14T10:30:01.500Z",
  "command_id": "cmd_001234",
  "action": "aroma",
  "status": "completed",
  "execution_time": 1450,
  "params_executed": {
    "pump": 2,
    "duration": 3000,
    "intensity": 80
  }
}
```

#### 3.4 Sincronización Temporal

**Topic**: `motete/partitura/sync`
**QoS**: 1 (sincronización crítica)

```json
{
  "timestamp": "2025-01-14T10:30:00.000Z",
  "sync_id": "sync_001",
  "partitura": "motete_principal",
  "position": "01:45.230",
  "bpm": 120,
  "beat": 4,
  "measure": 12
}
```

### 4. Protocolo de Comunicación

#### 4.1 Secuencia de Conexión
1. **Osmo se conecta** → Publica en `motete/osmo/discovery`
2. **Director detecta** → Responde con configuración
3. **Osmo confirma** → Publica estado inicial
4. **Director registra** → Unidad disponible para partituras

#### 4.2 Ejecución de Partitura
1. **Director lee YAML** → Parsea eventos programados
2. **Sincronización** → Envía pulso de tiempo
3. **Comando** → Envía acción específica a Osmo
4. **Confirmación** → Osmo confirma ejecución
5. **Monitoreo** → Director verifica estado

#### 4.3 Manejo de Errores
- **Timeout de comando**: Reenvío automático (máx. 3 intentos)
- **Osmo desconectado**: Pausa partitura, alerta al operador
- **Error de hardware**: Osmo reporta error, Director adapta partitura

### 5. Configuración del Broker Mosquitto

#### 5.1 Archivo de Configuración
```conf
# mosquitto.conf
port 1883
allow_anonymous false
password_file /etc/mosquitto/passwd

# Logs
log_dest file /var/log/mosquitto/mosquitto.log
log_type all

# Persistencia
persistence true
persistence_location /var/lib/mosquitto/

# Calidad de servicio
max_queued_messages 1000
message_size_limit 8192

# Tiempo de vida de conexión
keepalive_interval 60
```

#### 5.2 Usuarios y Permisos
```
# Archivo de usuarios
director:contraseña_segura_director
osmo_norte:contraseña_osmo_norte
osmo_sur:contraseña_osmo_sur  
osmo_este:contraseña_osmo_este
```

### 6. Integración con Node.js

#### 6.1 Cliente MQTT en Node.js
```javascript
const mqtt = require('mqtt');

class OsmoMQTTClient {
  constructor() {
    this.client = mqtt.connect('mqtt://localhost:1883', {
      username: 'director',
      password: 'contraseña_segura_director',
      clientId: 'director_' + Math.random().toString(16).substr(2, 8)
    });
    
    this.setupEventHandlers();
  }
  
  setupEventHandlers() {
    this.client.on('connect', () => {
      console.log('Director conectado al broker MQTT');
      this.subscribeToOsmoTopics();
    });
    
    this.client.on('message', (topic, message) => {
      this.handleIncomingMessage(topic, JSON.parse(message.toString()));
    });
  }
  
  subscribeToOsmoTopics() {
    this.client.subscribe('motete/osmo/+/status');
    this.client.subscribe('motete/osmo/+/actions');
    this.client.subscribe('motete/osmo/+/errors');
    this.client.subscribe('motete/osmo/discovery');
  }
  
  sendCommand(unitId, command) {
    const topic = `motete/director/commands/${unitId}`;
    this.client.publish(topic, JSON.stringify(command), { qos: 1 });
  }
}
```

### 7. Sistema de Estados

#### 7.1 Estados del Sistema
- `initializing` - Sistema iniciando
- `ready` - Listo para partituras
- `playing` - Ejecutando partitura
- `paused` - Partitura pausada
- `maintenance` - Modo mantenimiento
- `error` - Error del sistema

#### 7.2 Estados de Osmos Individuales
- `offline` - Desconectado
- `connecting` - Conectándose
- `ready` - Listo para comandos
- `busy` - Ejecutando acción
- `maintenance` - En mantenimiento
- `error` - Error de hardware

### 8. Monitoreo y Diagnóstico

#### 8.1 Dashboard de Estado
La interfaz web mostrará en tiempo real:
- Estado de conexión de cada Osmo
- Niveles de líquido aromático
- Temperatura de calentadores
- Historial de comandos ejecutados
- Latencia de red

#### 8.2 Logging
```javascript
// Estructura de logs
{
  "timestamp": "2025-01-14T10:30:00.000Z",
  "level": "info",
  "component": "mqtt_broker",
  "unit_id": "osmo_norte",
  "event": "command_executed",
  "details": {
    "command_id": "cmd_001234",
    "latency_ms": 45,
    "success": true
  }
}
```

### 9. Seguridad

#### 9.1 Autenticación
- Usuarios únicos por componente
- Contraseñas seguras generadas automáticamente
- Renovación periódica de credenciales

#### 9.2 Red Aislada
- WiFi dedicado sin acceso a internet
- Firewall que bloquea tráfico externo
- Encriptación WPA3

### 10. Escalabilidad

#### 10.1 Futuras Expansiones
- Soporte para más de 3 Osmos
- Integración con sensores ambientales
- Control de iluminación teatral
- Grabación de sesiones para análisis

#### 10.2 API REST Complementaria
Para integraciones externas:
```
GET  /api/osmos/status
POST /api/partitura/load
POST /api/partitura/play
PUT  /api/osmo/{id}/command
```

---

## Próximos Pasos

1. **Implementar broker Mosquitto** en Raspberry Pi
2. **Desarrollar cliente MQTT** en Node.js
3. **Crear firmware ESP32** para Osmos
4. **Diseñar protocolo de descobrimiento** automático
5. **Implementar parser YAML** para partituras
6. **Desarrollar interfaz web** de monitoreo

Este diseño proporciona una base sólida y escalable para el sistema de comunicación del "Motete transensorial".

# Task Flows - Plantilla Modular Osmo v5

## 1. Task Flow - Conexión WiFi

### Descripción
Flujo de tareas para establecer conexión WiFi en el dispositivo Osmo v5.

### Entrada
- Configuración WiFi (SSID y contraseña)
- Timeout de 20 segundos

### Proceso

```mermaid
flowchart TD
    A[Inicio] --> B[Configurar WiFi]
    B --> C[WiFi.begin(ssid, password)]
    C --> D[Iniciar timer de 20s]
    D --> E{WiFi.status() == WL_CONNECTED?}
    E -->|Sí| F[Mostrar IP local]
    E -->|No| G{Timer < 20s?}
    G -->|Sí| H[Delay 500ms]
    H --> I[yield()]
    I --> J[Mostrar "."]
    J --> E
    G -->|No| K[Mostrar timeout]
    F --> L[✅ WiFi conectado]
    K --> M[❌ Timeout WiFi - continuando sin conexión]
    L --> N[Fin]
    M --> N
```

### Salida
- **Éxito**: Conexión establecida, IP asignada
- **Fallo**: Timeout alcanzado, sistema continúa sin WiFi

### Código de referencia
```cpp
void NetworkManager::setupWiFi() {
    Serial.print("Conectando a WiFi...");
    
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    
    // TIMEOUT de 20 segundos
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 20000) {
        delay(500);
        yield();
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi conectado");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ Timeout WiFi - continuando sin conexión");
    }
}
```

---

## 2. Task Flow - Conexión MQTT

### Descripción
Flujo de tareas para establecer conexión MQTT con el broker.

### Entrada
- Configuración MQTT (servidor, puerto, credenciales)
- Timeout de 10 segundos
- WiFi previamente conectado

### Proceso

```mermaid
flowchart TD
    A[Inicio] --> B[Configurar servidor MQTT]
    B --> C[Iniciar timer de 10s]
    C --> D{!mqttClient.connected() && timer < 10s?}
    D -->|Sí| E[Intentar conectar MQTT]
    E --> F{mqttClient.connect() exitoso?}
    F -->|Sí| G[Mostrar "✅ MQTT conectado"]
    F -->|No| H[Mostrar error con código]
    H --> I[Delay 1000ms]
    I --> J[yield()]
    J --> D
    F -->|Sí| K[isConnected = true]
    K --> L[Suscribirse a topic de comandos]
    L --> M[Fin exitoso]
    D -->|No| N[Mostrar timeout MQTT]
    N --> O[Fin con error]
```

### Salida
- **Éxito**: Conexión MQTT establecida, suscripción a comandos
- **Fallo**: Timeout alcanzado, sistema continúa sin MQTT

### Código de referencia
```cpp
void NetworkManager::setupMQTT() {
    unsigned long startTime = millis();
    while (!mqttClient.connected() && (millis() - startTime) < 10000) {
        Serial.print("Intentando MQTT...");
        
        if (mqttClient.connect(mqttConfig.clientId, mqttConfig.user, mqttConfig.password)) {
            Serial.println("✅ MQTT conectado");
            isConnected = true;
            // Suscribirse a comandos del director
            char commandTopic[50];
            sprintf(commandTopic, "motete/director/commands/%s", deviceConfig.unitId);
            subscribe(commandTopic);
            
            return;
        } else {
            Serial.print("❌ MQTT falló, rc=");
            Serial.println(mqttClient.state());
            delay(1000);
            yield();
        }
    }
    
    if (!mqttClient.connected()) {
        Serial.println("❌ Timeout MQTT - continuando sin conexión");
    }
}
```

---

## 3. Task Flow - Publicación de Estado

### Descripción
Flujo de tareas para publicar el estado del dispositivo en el broker MQTT.

### Entrada
- Estado actual de las bombas
- Configuración del dispositivo
- Conexión MQTT activa

### Proceso

```mermaid
flowchart TD
    A[Inicio] --> B{MQTT conectado?}
    B -->|No| C[Mostrar error - MQTT desconectado]
    B -->|Sí| D[Crear JSON de estado]
    D --> E[Incluir unit_id y status]
    E --> F[Iterar sobre todas las bombas]
    F --> G[Obtener estado de cada bomba]
    G --> H[Obtener disponibilidad de cada bomba]
    H --> I[Agregar datos de bomba al JSON]
    I --> J{Más bombas?}
    J -->|Sí| F
    J -->|No| K[Serializar JSON]
    K --> L[Construir topic de estado]
    L --> M[Publicar con QoS 1]
    M --> N{Publicación exitosa?}
    N -->|Sí| O[Mostrar "✅ Estado publicado"]
    N -->|No| P[Mostrar "❌ Error al publicar"]
    O --> Q[Fin exitoso]
    P --> R[Fin con error]
    C --> R
```

### Salida
- **Éxito**: Estado publicado en topic `motete/osmo/{unitId}/status`
- **Fallo**: Error de conexión o publicación

### Código de referencia
```cpp
void StatusPublisher::publishStatus() {
    String statusJSON = createStatusJSON();
    char topic[50];
    sprintf(topic, "motete/osmo/%s/status", deviceConfig.unitId);
    
    // Publicar con QoS 1 para garantizar entrega
    if (networkManager->publishWithQoS(topic, statusJSON.c_str(), 1)) {
        Serial.println("✅ Estado publicado correctamente");
    } else {
        Serial.println("❌ Error al publicar estado");
    }
}

String StatusPublisher::createStatusJSON() {
    StaticJsonDocument<300> doc;
    
    doc["unit_id"] = deviceConfig.unitId;
    doc["status"] = "ready";
    
    // Incluir todas las bombas con su estado
    JsonObject pumps = doc.createNestedObject("pumps");
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        JsonObject pump = pumps.createNestedObject(String(i));
        bool pumpState = pumpController->getPumpState(i);
        pump["active"] = pumpState;
        pump["available"] = pumpController->isPumpAvailable(i);
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}
```

---

## 4. Task Flow - Recepción de Comandos

### Descripción
Flujo de tareas para recibir, validar y procesar comandos MQTT.

### Entrada
- Mensaje MQTT en topic de comandos
- Callback configurado en MQTT client

### Proceso

```mermaid
flowchart TD
    A[Mensaje MQTT recibido] --> B[Activar LED indicador]
    B --> C[Extraer mensaje del payload]
    C --> D[Parsear JSON del comando]
    D --> E{JSON válido?}
    E -->|No| F[Crear respuesta de error]
    E -->|Sí| G[Extraer commandId, action, params]
    G --> H[Validar parámetros del comando]
    H --> I{Parámetros válidos?}
    I -->|No| J[Crear respuesta INVALID_PARAMS]
    I -->|Sí| K{Acción del comando}
    K -->|ACTIVATE_PUMP| L[Verificar disponibilidad de bomba]
    L --> M{Bomba disponible o force?}
    M -->|No| N[Crear respuesta PUMP_BUSY]
    M -->|Sí| O[Activar bomba]
    O --> P[Crear respuesta SUCCESS]
    K -->|DEACTIVATE_PUMP| Q[Desactivar bomba]
    Q --> P
    K -->|GET_STATUS| R[Publicar estado actual]
    R --> P
    K -->|SET_PUMP_CONFIG| S[Actualizar configuración de bomba]
    S --> P
    K -->|REBOOT| T[Enviar respuesta y reiniciar]
    T --> U[ESP.restart()]
    K -->|RESET_CONFIG| V[Resetear configuración]
    V --> P
    K -->|Otro| W[Crear respuesta INVALID_COMMAND]
    F --> X[Enviar respuesta]
    J --> X
    N --> X
    P --> X
    W --> X
    X --> Y[Desactivar LED indicador]
    Y --> Z[Fin]
    U --> Z
```

### Salida
- **Éxito**: Comando procesado, respuesta enviada
- **Fallo**: Error de validación o procesamiento, respuesta de error enviada

### Código de referencia
```cpp
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
    
    handleCommand(topic, message.c_str());
}

void MainController::processCommand(const MQTTCommand& cmd) {
    // Validar parámetros primero
    if (!validateCommandParams(cmd)) {
        CommandResponse errorResponse = createResponse(ResponseCodes::INVALID_PARAMS, ErrorMessages::INVALID_PARAMS, cmd.commandId);
        sendCommandResponse(errorResponse);
        return;
    }
    
    // Procesar comando según la acción
    if (cmd.action == Commands::ACTIVATE_PUMP) {
        PumpActivationParams params = extractPumpActivationParams(cmd.params);
        
        // Verificar si la bomba está disponible
        if (!pumpController.isPumpAvailable(params.pumpId) && !params.force) {
            CommandResponse errorResponse = createResponse(ResponseCodes::PUMP_BUSY, ErrorMessages::PUMP_BUSY, cmd.commandId);
            sendCommandResponse(errorResponse);
            return;
        }
        
        // Activar bomba
        pumpController.setPumpState(params.pumpId, true);
        CommandResponse successResponse = createResponse(ResponseCodes::SUCCESS, SuccessMessages::PUMP_ACTIVATED, cmd.commandId);
        sendCommandResponse(successResponse);
    }
    // ... otros comandos
}
```

---

## Comandos Disponibles

### 1. ACTIVATE_PUMP
- **Parámetros**: `pump_id`, `duration`, `force`
- **Validación**: ID de bomba válido, duración entre 100ms-60s
- **Acción**: Activa bomba por tiempo especificado

### 2. DEACTIVATE_PUMP
- **Parámetros**: `pump_id`
- **Validación**: ID de bomba válido
- **Acción**: Desactiva bomba inmediatamente

### 3. GET_STATUS
- **Parámetros**: Ninguno
- **Acción**: Publica estado actual del dispositivo

### 4. SET_PUMP_CONFIG
- **Parámetros**: `pump_id`, `activation_time`, `cooldown_time`
- **Validación**: ID válido, tiempos entre 1s-5min
- **Acción**: Actualiza configuración de bomba

### 5. REBOOT
- **Parámetros**: Ninguno
- **Acción**: Reinicia el dispositivo

### 6. RESET_CONFIG
- **Parámetros**: Ninguno
- **Acción**: Restablece configuración a valores por defecto

---

## Códigos de Respuesta

- **200**: SUCCESS - Comando ejecutado correctamente
- **400**: INVALID_COMMAND - Comando no reconocido
- **404**: PUMP_NOT_FOUND - Bomba no encontrada
- **422**: INVALID_PARAMS - Parámetros inválidos
- **423**: PUMP_BUSY - Bomba en cooldown
- **500**: INTERNAL_ERROR - Error interno del sistema
- **503**: NETWORK_ERROR - Error de red

---

## Topics MQTT

### Suscripciones
- `motete/director/commands/{unitId}` - Comandos del director

### Publicaciones
- `motete/osmo/{unitId}/status` - Estado del dispositivo
- `motete/osmo/{unitId}/response` - Respuestas a comandos
- `motete/osmo/{unitId}/errors` - Errores del sistema
- `motete/osmo/{unitId}/heartbeat` - Latido del dispositivo
- `motete/osmo/{unitId}/test` - Mensajes de prueba

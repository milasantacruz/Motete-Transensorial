#include "network_manager.h"
#include <ArduinoJson.h>
#include <time.h>

// Función para sincronizar tiempo con NTP (necesario para TLS)
void NetworkManager::setCurrentTime() {
    Serial.println("Sincronizando tiempo con NTP...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    Serial.print("Esperando sincronización NTP");
    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 8 * 3600 * 2 && attempts < 20) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        attempts++;
    }
    Serial.println();
    
    if (now < 8 * 3600 * 2) {
        Serial.println("ERROR: No se pudo sincronizar el tiempo!");
    } else {
        struct tm timeinfo;
        gmtime_r(&now, &timeinfo);
        Serial.print("Tiempo sincronizado: ");
        Serial.println(asctime(&timeinfo));
    }
} 

NetworkManager::NetworkManager() : mqttClient(espClient), isConnected(false) {
    // Configurar certificados para AWS IoT Core (ESP8266 Core 3.1.2+)
    // Crear objetos X509List y PrivateKey desde strings
    static X509List caCert(awsConfig.caCert);
    static X509List deviceCert(awsConfig.deviceCert);
    static PrivateKey privateKey(awsConfig.privateKey);
    
    // CRÍTICO: Configurar buffers BearSSL para ESP8266
    espClient.setBufferSizes(256, 256); // rx_buffer, tx_buffer (reducidos)
    
    espClient.setTrustAnchors(&caCert);
    espClient.setClientRSACert(&deviceCert, &privateKey);
    
    // Configurar servidor AWS IoT Core
    mqttClient.setServer(awsConfig.endpoint, awsConfig.port);
    
    // CRÍTICO: Configurar buffer y timeout de PubSubClient
    mqttClient.setBufferSize(256); // Reducido para ahorrar memoria
    mqttClient.setSocketTimeout(15);
    mqttClient.setKeepAlive(60); // Keep-alive de 60 segundos
}

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
        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        // CRÍTICO: Sincronizar tiempo con NTP (necesario para TLS)
        setCurrentTime();
    } else {
        Serial.println("\n❌ Timeout WiFi - continuando sin conexión");
    }
}

void NetworkManager::setupMQTT() {
    Serial.println("🔐 Configurando conexión AWS IoT Core...");
    Serial.print("Endpoint: ");
    Serial.println(awsConfig.endpoint);
    Serial.print("Puerto: ");
    Serial.println(awsConfig.port);
    Serial.print("Thing Name: ");
    Serial.println(awsConfig.thingName);
    Serial.print("Memoria libre: ");
    Serial.println(ESP.getFreeHeap());
    
    // Verificar conectividad TCP
    Serial.print("Probando conectividad TCP a ");
    Serial.print(awsConfig.endpoint);
    Serial.print(":");
    Serial.println(awsConfig.port);
    
    WiFiClient testClient;
    if (testClient.connect(awsConfig.endpoint, awsConfig.port)) {
        Serial.println("✅ Conectividad TCP exitosa");
        testClient.stop();
    } else {
        Serial.println("❌ Error de conectividad TCP");
    }
    
    unsigned long startTime = millis();
    int attemptCount = 0;
    
    while (!mqttClient.connected() && (millis() - startTime) < 30000) {  // 30 segundos timeout
        attemptCount++;
        Serial.print("Intento #");
        Serial.print(attemptCount);
        Serial.print(" - Conectando a AWS IoT Core...");
        
        // AWS IoT Core no requiere usuario/contraseña, solo certificados
        // Probar con Client ID más simple
        if (mqttClient.connect("ESP82_Client")) {
            Serial.println("✅ AWS IoT Core conectado");
            isConnected = true;
            
            // Suscribirse solo a topics básicos (sin Device Shadow)
            char commandTopic[100];
            sprintf(commandTopic, "motete/director/commands/%s", deviceConfig.unitId);
            Serial.print("Suscribiéndose a: ");
            Serial.println(commandTopic);
            subscribe(commandTopic);
            
            return;
        } else {
            int state = mqttClient.state();
            Serial.print("❌ AWS IoT Core falló, rc=");
            Serial.print(state);
            Serial.print(" (");
            
            // Explicar códigos de error
            switch(state) {
                case -4: Serial.print("MQTT_CONNECTION_TIMEOUT"); break;
                case -3: Serial.print("MQTT_CONNECTION_LOST"); break;
                case -2: Serial.print("MQTT_CONNECT_FAILED"); break;
                case -1: Serial.print("MQTT_DISCONNECTED"); break;
                case 1: Serial.print("MQTT_CONNECT_BAD_PROTOCOL"); break;
                case 2: Serial.print("MQTT_CONNECT_BAD_CLIENT_ID"); break;
                case 3: Serial.print("MQTT_CONNECT_UNAVAILABLE"); break;
                case 4: Serial.print("MQTT_CONNECT_BAD_CREDENTIALS"); break;
                case 5: Serial.print("MQTT_CONNECT_UNAUTHORIZED"); break;
                default: Serial.print("UNKNOWN_ERROR"); break;
            }
            Serial.println(")");
            
            delay(3000); // Aumentar delay entre intentos
            yield();
        }
    }
    
    if (!mqttClient.connected()) {
        Serial.println("❌ Timeout AWS IoT Core - continuando sin conexión");
    }
}

bool NetworkManager::connect() {
    setupWiFi();
    setupMQTT();
    return isConnected;
}

bool NetworkManager::isMQTTConnected() {
    return mqttClient.connected();
}

void NetworkManager::loop() {
    if (!mqttClient.connected()) {
        isConnected = false;
        Serial.println("�� MQTT desconectado, reconectando...");
        // Intentar reconectar con delay para evitar spam
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        
        if (now - lastReconnectAttempt > 5000) { // Reintentar cada 5 segundos
            lastReconnectAttempt = now;
            setupMQTT();
        }
    }
    
    // CRÍTICO: Llamar mqttClient.loop() primero para mantener conexión
    mqttClient.loop();
    
    // Log de estado cada 60 segundos (menos frecuente)
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 60000) {
        Serial.print("📡 Estado MQTT: ");
        Serial.println(mqttClient.connected() ? "Conectado" : "Desconectado");
        lastLog = millis();
    }
}

bool NetworkManager::publish(const char* topic, const char* message) {
    return mqttClient.publish(topic, message);
}

bool NetworkManager::subscribe(const char* topic) {
    if (mqttClient.connected()) {
        bool result = mqttClient.subscribe(topic);
        if (result) {
            Serial.print("✅ Suscrito a: ");
            Serial.println(topic);
        } else {
            Serial.print("❌ Error suscribiéndose a: ");
            Serial.println(topic);
        }
        return result;
    }
    Serial.println("❌ MQTT no conectado para suscribirse");
    return false;
}

bool NetworkManager::publishWithQoS(const char* topic, const char* message, int qos) {
    if (mqttClient.connected()) {
        // Verificar longitud del mensaje
        int messageLength = strlen(message);
        if (messageLength > 512) {
            Serial.print("⚠️ Mensaje muy largo (");
            Serial.print(messageLength);
            Serial.println(" bytes), truncando...");
        }
        
        Serial.print("📤 Intentando publicar en ");
        Serial.print(topic);
        Serial.print(" (");
        Serial.print(messageLength);
        Serial.println(" bytes)");
        
        // Procesar mensajes pendientes antes de publicar
        if (mqttClient.loop()) {
            Serial.println("🔄 Procesando mensaje pendiente...");
            delay(10); // Pequeña pausa para procesar
        }
        
        bool result = mqttClient.publish(topic, message, qos);
        if (result) {
            Serial.print("✅ Publicado (QoS ");
            Serial.print(qos);
            Serial.print(") en ");
            Serial.print(topic);
            Serial.print(": ");
            Serial.println(message);
        } else {
            Serial.print("❌ Error publicando (QoS ");
            Serial.print(qos);
            Serial.print(") en ");
            Serial.println(topic);
            Serial.print("📊 Estado MQTT: ");
            Serial.println(mqttClient.state());
            Serial.print("📊 Buffer disponible: ");
            Serial.println(mqttClient.getBufferSize());
            Serial.print("📊 Mensaje pendiente: ");
            Serial.println(mqttClient.loop() ? "Sí" : "No");
            
            // Intentar limpiar el buffer
            Serial.println("🧹 Limpiando buffer MQTT...");
            for (int i = 0; i < 5; i++) {
                mqttClient.loop();
                delay(10);
            }
        }
        return result;
    }
    Serial.println("❌ MQTT no conectado para publicar");
    Serial.print("📊 Estado MQTT: ");
    Serial.println(mqttClient.state());
    return false;
}

void NetworkManager::publishError(const char* errorType, const char* message) {
    // Publicar error en topic de AWS IoT Core
    char topic[100];
    sprintf(topic, "$aws/things/%s/errors", awsConfig.thingName);
    
    StaticJsonDocument<256> doc;
    doc["timestamp"] = millis();
    doc["error_type"] = errorType;
    doc["message"] = message;
    doc["unit_id"] = deviceConfig.unitId;
    
    String errorJSON;
    serializeJson(doc, errorJSON);
    
    publishWithQoS(topic, errorJSON.c_str(), 1);
}

bool NetworkManager::testConnection() {
    if (mqttClient.connected()) {
        // Publicar mensaje de test en AWS IoT Core
        char topic[100];
        sprintf(topic, "$aws/things/%s/test", awsConfig.thingName);
        return publish(topic, "test_message");
    }
    return false;
}

void NetworkManager::sendHeartbeat() {
    if (mqttClient.connected()) {
        // Publicar heartbeat en AWS IoT Core Shadow
        char topic[100];
        sprintf(topic, "$aws/things/%s/shadow/update", awsConfig.thingName);
        
        StaticJsonDocument<256> doc;
        doc["state"]["reported"]["timestamp"] = millis();
        doc["state"]["reported"]["unit_id"] = deviceConfig.unitId;
        doc["state"]["reported"]["status"] = "alive";
        doc["state"]["reported"]["mqtt_connected"] = true;
        
        String heartbeatJSON;
        serializeJson(doc, heartbeatJSON);
        
        publishWithQoS(topic, heartbeatJSON.c_str(), 0); // QoS 0 para heartbeat
    }
}
//inicialización de callback
void NetworkManager::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    mqttClient.setCallback(callback);
}
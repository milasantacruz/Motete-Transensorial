#include "network_manager.h"
#include <ArduinoJson.h> 

NetworkManager::NetworkManager() : mqttClient(espClient), isConnected(false) {
    mqttClient.setServer(mqttConfig.server, mqttConfig.port);
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
    } else {
        Serial.println("\n❌ Timeout WiFi - continuando sin conexión");
    }
}

void NetworkManager::setupMQTT() {
   // mqttClient.setKeepAlive(mqttConfig.keepAlive);
    //mqttClient.setCleanSession(mqttConfig.cleanSession);

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
        setupMQTT();
    }
    
    mqttClient.loop();
    
    // Log de estado cada 30 segundos
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 30000) {
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
    char topic[100];
    sprintf(topic, "motete/osmo/%s/errors", deviceConfig.unitId);
    
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
        // Publicar mensaje de test
        char topic[50];
        sprintf(topic, "motete/osmo/%s/test", deviceConfig.unitId);
        return publish(topic, "test_message");
    }
    return false;
}

void NetworkManager::sendHeartbeat() {
    if (mqttClient.connected()) {
        char topic[50];
        sprintf(topic, "motete/osmo/%s/heartbeat", deviceConfig.unitId);
        
        StaticJsonDocument<128> doc;
        doc["timestamp"] = millis();
        doc["unit_id"] = deviceConfig.unitId;
        doc["status"] = "alive";
        
        String heartbeatJSON;
        serializeJson(doc, heartbeatJSON);
        
        publishWithQoS(topic, heartbeatJSON.c_str(), 0); // QoS 0 para heartbeat
    }
}
//inicialización de callback
void NetworkManager::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    mqttClient.setCallback(callback);
}
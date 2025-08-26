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
        Serial.printf("📡 Estado MQTT: %s\n", 
            mqttClient.connected() ? "Conectado" : "Desconectado");
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
            Serial.printf("✅ Suscrito a: %s\n", topic);
        } else {
            Serial.printf("❌ Error suscribiéndose a: %s\n", topic);
        }
        return result;
    }
    Serial.println("❌ MQTT no conectado para suscribirse");
    return false;
}

bool NetworkManager::publishWithQoS(const char* topic, const char* message, int qos) {
    if (mqttClient.connected()) {
        bool result = mqttClient.publish(topic, message, qos);
        if (result) {
            Serial.printf("✅ Publicado (QoS %d) en %s: %s\n", qos, topic, message);
        } else {
            Serial.printf("❌ Error publicando (QoS %d) en %s\n", qos, topic);
        }
        return result;
    }
    Serial.println("❌ MQTT no conectado para publicar");
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
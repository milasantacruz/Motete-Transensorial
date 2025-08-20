#include "network_manager.h"

NetworkManager::NetworkManager() : mqttClient(espClient), isConnected(false) {
    mqttClient.setServer(mqttConfig.server, mqttConfig.port);
}

void NetworkManager::setupWiFi() {
    Serial.print("Conectando a ");
    Serial.println(wifiConfig.ssid);
    
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void NetworkManager::setupMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Intentando conexión MQTT...");
        
        if (mqttClient.connect(mqttConfig.clientId, mqttConfig.user, mqttConfig.password)) {
            Serial.println("conectado");
            isConnected = true;
            
            // Suscribirse al topic de comandos
            char commandTopic[50];
            sprintf(commandTopic, "motete/director/commands/%s", deviceConfig.unitId);
            subscribe(commandTopic);
            
            return;
        } else {
            Serial.print("falló, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" intentando de nuevo en 5 segundos");
            delay(5000);
        }
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
        setupMQTT();
    }
    mqttClient.loop();
}

bool NetworkManager::publish(const char* topic, const char* message) {
    return mqttClient.publish(topic, message);
}

bool NetworkManager::subscribe(const char* topic) {
    return mqttClient.subscribe(topic);
}

void NetworkManager::setCallback(void (*callback)(char*, byte*, unsigned int)) {
    mqttClient.setCallback(callback);
}
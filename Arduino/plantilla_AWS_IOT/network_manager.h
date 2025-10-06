#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <BearSSLHelpers.h>
#include "config.h"



class NetworkManager {
private:
    WiFiClientSecure espClient;  // Cambiar a WiFiClientSecure para AWS IoT Core
    PubSubClient mqttClient;
    bool isConnected;
    
    void setupWiFi();
    void setupMQTT();
    void setCurrentTime(); // Sincronización NTP
    
public:
    NetworkManager();
    bool connect();
    bool isMQTTConnected();
    void loop();
    bool publish(const char* topic, const char* message);
    bool subscribe(const char* topic);
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));

    bool publishWithQoS(const char* topic, const char* message, int qos = 1);
    void publishError(const char* errorType, const char* message);
    bool testConnection();
    void sendHeartbeat();
};

#endif
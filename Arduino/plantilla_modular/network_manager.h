#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

class NetworkManager {
private:
    WiFiClient espClient;
    PubSubClient mqttClient;
    bool isConnected;
    
    void setupWiFi();
    void setupMQTT();
    
public:
    NetworkManager();
    bool connect();
    bool isMQTTConnected();
    void loop();
    bool publish(const char* topic, const char* message);
    bool subscribe(const char* topic);
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));
};

#endif
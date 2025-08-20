#ifndef CONFIG_H
#define CONFIG_H

// Configuración WiFi
struct WiFiConfig {
    const char* ssid;
    const char* password;
};

// Configuración MQTT
struct MQTTConfig {
    const char* server;
    int port;
    const char* user;
    const char* password;
    const char* clientId;
};

// Configuración del dispositivo
struct DeviceConfig {
    const char* unitId;
    int pumpCount;
    int statusInterval;
};

// Configuración global
extern WiFiConfig wifiConfig;
extern MQTTConfig mqttConfig;
extern DeviceConfig deviceConfig;

#endif
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
    int qos; // (0=sin garantía, 1=al menos una vez, 2=exactamente una vez)           
    int keepAlive;   //Tiempo en segundos entre mensajes de "estoy vivo"   
    bool cleanSession; //Si es true, el broker olvida la sesión anterior al reconectar
};

// Configuración por defecto de bombas
struct PumpDefaultConfig {
    int activationTime;
    int cooldownTime;
};

// Configuración del dispositivo
struct DeviceConfig {
    const char* unitId;
    int pumpCount;
    int statusInterval;
    int pumpPins[4];
    PumpDefaultConfig pumpDefaults;
};

// Configuración global
extern WiFiConfig wifiConfig;
extern MQTTConfig mqttConfig;
extern DeviceConfig deviceConfig;

#endif
#ifndef CONFIG_H
#define CONFIG_H

// Configuración WiFi
struct WiFiConfig {
    const char* ssid;
    const char* password;
};

// Configuración AWS IoT Core
struct AWSConfig {
    const char* endpoint;        // Endpoint de AWS IoT Core
    int port;                   // 8883 para MQTT over TLS
    const char* thingName;      // Nombre del dispositivo en AWS
    const char* caCert;         // Certificado CA de AWS
    const char* deviceCert;     // Certificado del dispositivo
    const char* privateKey;     // Clave privada del dispositivo
    int qos;                    // QoS por defecto
    int keepAlive;              // Tiempo en segundos entre mensajes de "estoy vivo"
    bool cleanSession;          // Si es true, el broker olvida la sesión anterior al reconectar
};

// Configuración MQTT (mantener para compatibilidad)
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
extern AWSConfig awsConfig;
extern MQTTConfig mqttConfig;
extern DeviceConfig deviceConfig;

#endif
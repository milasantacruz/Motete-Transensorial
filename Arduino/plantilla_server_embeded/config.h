#ifndef CONFIG_H
#define CONFIG_H

// Configuración WiFi
struct WiFiConfig {
    const char* ssid;
    const char* password;
};

// Configuración del servidor web
struct WebServerConfig {
    int port;
    const char* hostname;
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
extern WebServerConfig webServerConfig;
extern DeviceConfig deviceConfig;

#endif

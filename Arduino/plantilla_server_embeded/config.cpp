#include "config.h"

// Configuración WiFi
WiFiConfig wifiConfig = {
    "FreakStudio_TPLink",           // SSID de tu red WiFi
    "Freaknoize2025"        // Contraseña de tu red WiFi
};

// Configuración del servidor web
WebServerConfig webServerConfig = {
    80,                 // Puerto del servidor web
    "osmo-piano"        // Hostname del dispositivo
};

// Configuración del dispositivo
DeviceConfig deviceConfig = {
    "OSMO_PIANO_001",   // ID único del dispositivo
    4,                  // Número de bombas
    2000,               // Intervalo de publicación de estado (ms)
    {12,13,14,15},      // Pines de las bombas (GPIO)
    {1000, 3000}        // Tiempo de activación y cooldown por defecto (ms)
};

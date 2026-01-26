#include "config.h"

// Configuración WiFi
WiFiConfig wifiConfig = {
    .ssid = "Personal-926", //"FreakStudio_TPLink",
    .password = "6TJv6nwtHn" // "Freaknoize2025"
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
    {2000, 3000}        // Tiempo de activación y cooldown por defecto (ms)
};

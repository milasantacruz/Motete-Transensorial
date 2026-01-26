#ifndef CONFIG_H
#define CONFIG_H

// Credenciales Wi-Fi
const char* ssid = "FreakStudio_TPLink";
const char* password = "Freaknoize2025";

// Configuración de IP estática
IPAddress local_IP(192, 168, 0, 211);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

#endif


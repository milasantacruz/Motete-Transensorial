#include "config.h"

WiFiConfig wifiConfig = {
    .ssid = "FreakStudio_TPLink",
    .password = "Freaknoize2025"
};

MQTTConfig mqttConfig = {
    .server = "192.168.1.34",
    .port = 1883,
    .user = "osmo_norte",
    .password = "norte",
    .clientId = "osmo_norte",
    .qos = 1, // QoS 1 para garantizar entrega
    .keepAlive = 60,   
    .cleanSession = true
};

DeviceConfig deviceConfig = {
    .unitId = "osmo_norte",
    .pumpCount = 4,  // ✅ Cambiado a 4 para tener bombas 0, 1, 2, 3
    .statusInterval = 10000,
    .pumpPins = {12,13,14,15},  // Pines más seguros para ESP8266
    .pumpDefaults = {
        .activationTime = 2000,  // 10 segundos por defecto
        .cooldownTime = 3000     // 30 segundos por defecto
    }
};
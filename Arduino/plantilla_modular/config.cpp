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
    .pumpCount = 2,
    .statusInterval = 10000,
    .pumpPins = {12,13,14,15}  // Pines más seguros para ESP8266
};
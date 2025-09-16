#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// Forward declaration
class PumpController;

class NetworkManager {
private:
    ESP8266WebServer* webServer;
    WebSocketsServer* webSocketServer;
    bool wifiConnected;
    bool serverStarted;
    unsigned long lastStatusCheck;
    
    // Referencia al controlador de bombas
    PumpController* pumpController;
    
    // Callbacks para el servidor web
    void handleRoot();
    void handlePiano();
    void handleCommonCSS();
    void handlePianoCSS();
    void handlePianoJS();
    void handleAPIStatus();
    void handleAPICommand();
    void handleNotFound();
    
    // Callback para WebSocket
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    
    // Método estático para el callback wrapper
    static NetworkManager* instance;
    static void webSocketEventWrapper(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    
public:
    NetworkManager();
    ~NetworkManager();
    
    bool initialize(PumpController* pumpCtrl);
    void loop();
    bool isConnected();
    void sendWebSocketMessage(const String& message);
    
    // Métodos para obtener datos del sistema
    String getSystemStatus();
    String getOsmoConfigs();
    void sendCommandResponse(const String& unitId, const String& response);
};

#endif

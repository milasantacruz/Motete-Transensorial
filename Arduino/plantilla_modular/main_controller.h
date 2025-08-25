#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "network_manager.h"
#include "pump_controller.h"
#include "status_publisher.h"

class MainController {
private:
    NetworkManager networkManager;
    PumpController pumpController;
    StatusPublisher statusPublisher;
    
    unsigned long lastStatusPublish;
    
    void handleCommand(const char* topic, const char* message);
    void publishStatus();
    
public:
    MainController();
    void initialize();
    void loop();
    // Método estático que redirige al método de instancia
    static void messageCallback(char* topic, uint8_t* payload, unsigned int length);
        
    // Método de instancia que procesa el mensaje
    void procesarMensaje(char* topic, uint8_t* payload, unsigned int length);};

#endif
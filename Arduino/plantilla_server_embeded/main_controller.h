#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

// Forward declarations para evitar problemas de inicialización
class NetworkManager;
class PumpController;

class MainController {
private:
    // Usar punteros para crear objetos dinámicamente después de Serial.begin()
    NetworkManager* networkManager;
    PumpController* pumpController;
    
    unsigned long lastStatusPublish;
    
public:
    MainController();
    ~MainController();
    void initialize();
    void loop();
};

#endif

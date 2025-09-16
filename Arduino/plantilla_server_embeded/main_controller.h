#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "network_manager.h"
#include "pump_controller.h"

class MainController {
private:
    NetworkManager networkManager;
    PumpController pumpController;
    
    unsigned long lastStatusPublish;
    
public:
    MainController();
    ~MainController();
    void initialize();
    void loop();
};

#endif

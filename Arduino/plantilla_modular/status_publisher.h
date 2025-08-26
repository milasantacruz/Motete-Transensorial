#ifndef STATUS_PUBLISHER_H
#define STATUS_PUBLISHER_H

#include <ArduinoJson.h>
#include "config.h"
#include "pump_controller.h"
#include "network_manager.h"

class StatusPublisher {
private:
    PumpController* pumpController;
    NetworkManager* networkManager;
    
public:
    StatusPublisher(PumpController* pumpCtrl, NetworkManager* netMgr);
    void publishStatus();
    String createStatusJSON();
};

#endif
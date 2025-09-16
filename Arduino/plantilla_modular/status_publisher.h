#ifndef STATUS_PUBLISHER_H
#define STATUS_PUBLISHER_H

#include <ArduinoJson.h>
#include "config.h"
#include "network_manager.h"

// Forward declaration para evitar dependencias circulares
class PumpController;

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
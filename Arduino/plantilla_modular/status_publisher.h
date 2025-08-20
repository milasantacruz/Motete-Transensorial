#ifndef STATUS_PUBLISHER_H
#define STATUS_PUBLISHER_H

#include <ArduinoJson.h>
#include "config.h"
#include "pump_controller.h"

class StatusPublisher {
private:
    PumpController* pumpController;
    
public:
    StatusPublisher(PumpController* pumpCtrl);
    void publishStatus();
    String createStatusJSON();
};

#endif
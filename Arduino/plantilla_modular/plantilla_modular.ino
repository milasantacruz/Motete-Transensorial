#include "main_controller.h"

MainController controller;

void setup() {
    controller.initialize();
}

void loop() {
    controller.loop();
}
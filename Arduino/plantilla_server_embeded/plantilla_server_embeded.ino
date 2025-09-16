/*
 * Motete Transensorial - Piano Server Embebido
 * 
 * Este proyecto es una versión simplificada de plantilla_modular que:
 * - No usa MQTT
 * - Levanta un servidor web embebido
 * - Sirve la página "piano" para controlar las bombas de aromas
 * 
 * Características:
 * - Servidor web en puerto 80
 * - WebSocket en puerto 81 para comunicación en tiempo real
 * - API REST para comandos y estado
 * - Control de bombas con cooldown
 * - Interfaz web con p5.js
 * 
 * Configuración:
 * - Editar config.cpp para WiFi y configuración del dispositivo
 * - Conectar bombas a los pines configurados
 * - Acceder a http://[IP_DEL_DISPOSITIVO]/piano.html
 */

#include "main_controller.h"

MainController controller;

void setup() {
    controller.initialize();
}

void loop() {
    controller.loop();
}

template<class T> inline Print &operator <<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}
/////////////LIBRERIAS///////////////
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <WebServer.h>
WiFiMulti wifiMulti;
WebServer server(80);
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
#endif
#ifdef __AVR__
#include <avr/power.h>
#endif
#include "data.h"
/////////////////////////////////////
////////////VARIABLES////////////////
int aroma1 = 2;
int aroma2 = 4;
int aroma3 = 5;
//const int pulsoTiempo= 5000;
const uint32_t TiempoEsperaWifi = 5000;
// Variables para almacenar el tiempo de activación de cada botón
unsigned long tiempoInicio1 = 0;
unsigned long tiempoInicio2 = 0;
unsigned long tiempoInicio3 = 0;
unsigned long tiempoActivacion1 = 10000;  // Tiempo configurable desde HTML (en ms)
unsigned long tiempoActivacion2 = 10000;
unsigned long tiempoActivacion3 = 10000;
boolean Estado1 = false;
boolean Estado2 = false;
boolean Estado3 = false;
String Valor = "";
/////////////////////////////////////
//////////////FUNCIONES//////////////
void mensajeBase() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", Pagina);
}
void funcionEncender1() {
  Estado1 = true;
  digitalWrite(aroma1, HIGH);
  tiempoInicio1 = millis();  // Guardamos el tiempo de activación
  Serial.print("AROMA 1: EMITIENDO (durante ");
  Serial.print(tiempoActivacion1 / 1000);
  Serial.println(" SEGUNDOS)");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Emitiendo AROMA 1");
}
void funcionEncender2() {
  Estado2 = true;
  digitalWrite(aroma2, HIGH);
  tiempoInicio2 = millis();  // Guardamos el tiempo de activación
  Serial.print("AROMA 2: EMITIENDO (durante ");
  Serial.print(tiempoActivacion2 / 1000);
  Serial.println(" SEGUNDOS)");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Emitiendo AROMA 2");
}
void funcionEncender3() {
  Estado3 = true;
  digitalWrite(aroma3, HIGH);
  tiempoInicio3 = millis();  // Guardamos el tiempo de activación
  Serial.print("AROMA 3: EMITIENDO (durante ");
  Serial.print(tiempoActivacion3 / 1000);
  Serial.println(" SEGUNDOS)");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Emitiendo AROMA 3");
}

// Funciones para configurar tiempos de activación
void setTiempo1() {
  if (server.hasArg("tiempo")) {
    tiempoActivacion1 = server.arg("tiempo").toInt() * 1000; // Convertir segundos a milisegundos
    Serial.print("Tiempo AROMA 1 configurado a: ");
    Serial.print(tiempoActivacion1 / 1000);
    Serial.println(" segundos");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Tiempo AROMA 1: " + String(tiempoActivacion1 / 1000) + "s");
}

void setTiempo2() {
  if (server.hasArg("tiempo")) {
    tiempoActivacion2 = server.arg("tiempo").toInt() * 1000; // Convertir segundos a milisegundos
    Serial.print("Tiempo AROMA 2 configurado a: ");
    Serial.print(tiempoActivacion2 / 1000);
    Serial.println(" segundos");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Tiempo AROMA 2: " + String(tiempoActivacion2 / 1000) + "s");
}

void setTiempo3() {
  if (server.hasArg("tiempo")) {
    tiempoActivacion3 = server.arg("tiempo").toInt() * 1000; // Convertir segundos a milisegundos
    Serial.print("Tiempo AROMA 3 configurado a: ");
    Serial.print(tiempoActivacion3 / 1000);
    Serial.println(" segundos");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Tiempo AROMA 3: " + String(tiempoActivacion3 / 1000) + "s");
}
/////////////////////////////////////
void setup() {
  //--Iniciando multiwifi--//
  Serial.begin(9600);
  Serial.println("\nIniciando Server Web");
  wifiMulti.addAP(ssid_1, password_1);
  wifiMulti.addAP(ssid_2, password_2);
  //--Pines (aromas) iniciando bajo--//
  pinMode(aroma1, OUTPUT);
  digitalWrite(aroma1, LOW);
  pinMode(aroma2, OUTPUT);
  digitalWrite(aroma2, LOW);
  pinMode(aroma3, OUTPUT);
  digitalWrite(aroma3, LOW);
  //--Confirmamos multiwifi encendido--//
  WiFi.mode(WIFI_STA);
  //--Asignar IP estática--//
  WiFi.config(IPAddress(192, 168, 0, 211),    // IP estática que quieres asignar
              IPAddress(192, 168, 0, 1),      // Dirección IP de la puerta de enlace (router)
              IPAddress(255, 255, 255, 0));   // Máscara de subred (comúnmente 255.255.255.0)
  Serial.print("Conectando a Wifi ..");
  //--Damos tiempo para conectar--//
  while (wifiMulti.run(TiempoEsperaWifi) != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println(".. Conectado");
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print(" ID: ");
  Serial.println(WiFi.localIP());
  //--Activamos el MDNS--//
  if (!MDNS.begin("elarbol")) {
    Serial.println("Erro configurando mDNS!");
    while (1) {
      delay(1000);
    }
  }

  //--Pagina de inicio--//
  server.on("/" , mensajeBase);
  //--Funcion encender--//
  server.on("/vivo1", funcionEncender1);
  server.on("/vivo2", funcionEncender2);
  server.on("/vivo3", funcionEncender3);
  //--ACA CONFIURA EL TIEMPO-//
  server.on("/setTiempo1", HTTP_GET, setTiempo1);
  server.on("/setTiempo2", HTTP_GET, setTiempo2);
  server.on("/setTiempo3", HTTP_GET, setTiempo3);
  //--Inicia server e  imprime puerto serial--//
  server.begin();
  Serial.println("Servidor HTTP iniciado");
  //-- --//
  Pagina.replace("%ip", WiFi.localIP().toString());
}

void loop() {
  //--Comprobar si han pasado 5 segundos para apagar los botones, sin bloquear el código
  if (Estado1 && millis() - tiempoInicio1 >= tiempoActivacion1) {
    digitalWrite(aroma1, LOW);
    Estado1 = false;
    Serial.println("AROMA 1: APAGADO");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Parando AROMA 1");
  }

  if (Estado2 && millis() - tiempoInicio2 >= tiempoActivacion2) {
    digitalWrite(aroma2, LOW);
    Estado2 = false;
    Serial.println("AROMA 2: APAGADO");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Parando AROMA 2");
  }

  if (Estado3 && millis() - tiempoInicio3 >= tiempoActivacion3) {
    digitalWrite(aroma3, LOW);
    Estado3 = false;
    Serial.println("AROMA 3: APAGADO");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Parando AROMA 3");
  }
  //--Cliente puede administrar web--//
  server.handleClient();
  //--Actualiza MDNS para el ESP8266--//
  #if defined(ESP8266)
    MDNS.update();
  #endif
}

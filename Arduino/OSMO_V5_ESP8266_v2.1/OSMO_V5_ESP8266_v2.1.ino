// LIBRERÍAS
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ------------------- CONFIGURACIÓN PERSONALIZABLE -------------------
const char* ssid = "FreakStudio_TPLink";             // Cambia por tu red WiFi
const char* password = "Freaknoize";     // Cambia por tu contraseña WiFi
const char* mqtt_server = "192.168.1.1"; // ip de la PC donde corre el Broker
const int mqtt_port = 1883; // puerto en donde trabaja el broker
const char* UNIT_ID = "osmo_norte";
const char* mqtt_user = "osmo_norte";
const char* mqtt_password = "norte"; // pass del ESP8266
const int pumpPins[8] = {5, 4, 0, 2, 14, 12, 13, 15}; // pines donde están las bombas

// ------------------- LÓGICA NO BLOQUEANTE -------------------
// Arrays para gestionar el estado y tiempo de cada bomba de forma independiente
bool pumpIsActive[8] = {false};
unsigned long pumpStopTime[8] = {0};

// Variable para gestionar el tiempo de reconexión a MQTT
long lastReconnectAttempt = 0;
// --------------------------------------------------------------------

// Clientes de Red y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Función para procesar los comandos recibidos
void processCommand(JsonObject doc) {
  const char* action = doc["action"];

  if (strcmp(action, "aroma") == 0) {
    int pumpIndex = doc["params"]["pump"];
    int duration = doc["params"]["duration"];

    if (pumpIndex >= 1 && pumpIndex <= 8) {
      Serial.printf("Iniciando bomba %d por %d ms\n", pumpIndex, duration);

      // --- LÓGICA DE ACTIVACIÓN NO BLOQUEANTE (MODIFICADO) ---
      // En lugar de usar delay, registramos el estado y el tiempo de parada
      int pinIndex = pumpIndex - 1;
      digitalWrite(pumpPins[pinIndex], LOW); // Activa la bomba (LOW es ON)
      pumpIsActive[pinIndex] = true;
      pumpStopTime[pinIndex] = millis() + duration; // Calcula cuándo debe apagarse
    }
  }
}

// Función para gestionar las bombas activas (NUEVO)
void handlePumps() {
  unsigned long currentTime = millis();
  for (int i = 0; i < 8; i++) {
    // Revisa si una bomba está activa y si ya pasó su tiempo de duración
    if (pumpIsActive[i] && currentTime >= pumpStopTime[i]) {
      digitalWrite(pumpPins[i], HIGH); // Apaga la bomba (HIGH es OFF)
      pumpIsActive[i] = false; // Marca la bomba como inactiva
      Serial.printf("Bomba %d completada.\n", i + 1);
    }
  }
}

// Callback: esta función se ejecuta cada vez que llega un mensaje
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el topic: ");
  Serial.println(topic);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("Error al parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }
  processCommand(doc.as<JsonObject>());
}

// Función para conectarse a la red WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Función para reconectar al Broker MQTT
void reconnect() {
  Serial.print("Intentando conexión MQTT...");
  if (client.connect(UNIT_ID, mqtt_user, mqtt_password)) {
    Serial.println("✅ Conectado al Broker MQTT");
    char commandTopic[50];
    sprintf(commandTopic, "motete/director/commands/%s", UNIT_ID);
    client.subscribe(commandTopic);
    Serial.print("Suscrito a: ");
    Serial.println(commandTopic);
  } else {
    Serial.print("falló, rc=");
    Serial.print(client.state());
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 8; i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH);
  }

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // --- LÓGICA DE RECONEXIÓN NO BLOQUEANTE ---
  if (!client.connected()) {
    long now = millis();
    // Intenta reconectar solo cada 5 segundos
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("Intentando reconexión MQTT...");
      reconnect(); // Intenta conectar una vez
    }
  } else {
     Serial.println("conectado...");
    // Solo procesa mensajes si está conectado
    client.loop();
  }

  // --- GESTIÓN DE BOMBAS NO BLOQUEANTE ---
  // Esta función se ejecuta en cada ciclo para verificar si hay que apagar alguna bomba
  handlePumps();
}
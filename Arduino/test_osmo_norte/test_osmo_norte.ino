#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ----------- CONFIGURACIÓN -----------
const char* ssid = "FreakStudio_TPLink";             // Cambia por tu red WiFi
const char* password = "Freaknoize2025";     // Cambia por tu contraseña WiFi (ejecutar ipconfig en la terminal, debe ser el valor de ipv4)
const char* mqtt_server = "192.168.1.34";  // Cambia por la IP de tu broker MQTT
const int mqtt_port = 1883;               // Cambia por el puerto de tu broker
const char* mqtt_user = "osmo_norte";     // Usuario MQTT
const char* mqtt_pass = "norte";     // Contraseña MQTT

WiFiClient espClient;
PubSubClient client(espClient);

// NUEVO: función para procesar los comandos recibidos
void callback(char* topic, byte* payload, unsigned int length) {
  // Enciende el LED incorporado (pin 2) al recibir mensaje
  digitalWrite(2, LOW);  // LOW enciende el LED en ESP8266
  delay(100);            // Mantiene encendido por 100ms
  digitalWrite(2, HIGH); // Apaga el LED
  
  Serial.print("Mensaje recibido en el topic: ");
  Serial.println(topic);

  String mensaje;
  for (uint16_t i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  Serial.print("Contenido: ");
  Serial.println(mensaje);
}

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
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void publishStatus() {
  // Crea el JSON de estado (ajusta los valores si lo deseas)
  StaticJsonDocument<512> doc;
  doc["timestamp"] = "2025-01-14T10:30:01.000Z";
  doc["unit_id"] = "osmo_norte";
  doc["status"] = "ready";
  JsonObject pumps = doc.createNestedObject("pumps");
  for (int i = 1; i <= 2; i++) {
    JsonObject pump = pumps.createNestedObject(String(i));
    pump["active"] = (i == 2); // Solo la bomba 2 activa como ejemplo
    pump["level"] = 80 + i;    // Niveles de ejemplo
  }
  doc["battery"] = random(0, 100);

  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  //(MQTT_MAX_PACKET_SIZE 128 en PubSubClient.h).
  Serial.printf("Longitud JSON = %u bytes\n", n);
  // Publica el mensaje
  boolean result = client.publish("motete/osmo/osmo_norte/status", buffer, n);
  Serial.printf("publish() -> %s\n", result ? "OK" : "FAIL");
  if (result) {
    Serial.println("Estado publicado correctamente.");
  } else {
    Serial.println("Error al publicar estado.");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("osmo_norte", mqtt_user, mqtt_pass)) {
      Serial.println("conectado");

      // NUEVO: suscripción al topic de comandos
      char commandTopic[50];
      sprintf(commandTopic, "motete/director/commands/%s", "osmo_norte");
      client.subscribe(commandTopic);
      Serial.print("Suscrito a: ");
      Serial.println(commandTopic);

      // Publica el estado al conectar
      publishStatus();
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configurar el LED incorporado (pin 2)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH); // Inicialmente apagado
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // NUEVO: establece la función de callback
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publica el estado cada 10 segundos
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 10000) {
    publishStatus();
    lastPublish = millis();
  }
}
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ----------- CONFIGURACIÓN -----------
const char* ssid = "FreakStudio_TPLink";             // Cambia por tu red WiFi
const char* password = "Freaknoize";     // Cambia por tu contraseña WiFi
const char* mqtt_server = "192.168.1.1";  // Cambia por la IP de tu broker MQTT
const int mqtt_port = 1883;               // Cambia por el puerto de tu broker
const char* mqtt_user = "osmo_norte";     // Cambia por tu usuario MQTT
const char* mqtt_pass = "norte";          // Cambia por tu contraseña MQTT

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Attempt to connect
    if (client.connect("ESP8266TestClient", mqtt_user, mqtt_pass)) {
      Serial.println("conectado");
      // Subscribe
      client.subscribe("test/topic");
      // Publish a test message
      client.publish("test/topic", "¡Hola desde ESP8266!");
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
  delay(10);
  Serial.println();
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
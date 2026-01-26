#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "config.h" // Credenciales Wi-Fi

WiFiServer server(80);

String header;

///////////DFPLAYER librerias y variables///////////////
#include "Arduino.h" // Include the core Arduino library
#include "DFRobotDFPlayerMini.h" // Include the DFRobot DFPlayer Mini library

#ifdef ESP32
  #define FPSerial Serial1  // For ESP32, use hardware serial port 1
#else
  #include <SoftwareSerial.h> // Include SoftwareSerial library for non-ESP32 boards
  SoftwareSerial FPSerial(16, 17); // Define SoftwareSerial on pins 16 (RX) and 17 (TX)
#endif

DFRobotDFPlayerMini myDFPlayer; // Create an instance of the DFRobotDFPlayerMini class

int reproducir = 1;  // se activa (1) al presionar Preset
int ancla = 1;  // orignalmente 1 para dar primera lectura al audio, luego siempre en 0
bool dfPlayerAvailable = false; // Estado del DFPlayer
///////////DFPLAYERFIN/////////////////////////////////
int esperaBombas = 0; // agrega espera al inicio de cada pulso de bombas
int esperaAudio = 0; // agrega espera al inicio del audio

// Variables
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 1000;

String bombaNames[8] = {
  //"B1 Cuero", "B2 Rayito", "B3 Depressing", "B4 Relaxing",
  //"B5 Humedad", "B6 Rosa", "B7 Ozono", "B8 Fresh Spirit"
  "Aroma 1", "Aroma 2", " Aroma 3", "Aroma 4",
  "Aroma 5", "Aroma 6", "Aroma 7", "Aroma 8"
};

int bombas[8] = {18, 19, 21, 22, 23, 25, 26, 27}; // Pines del esp32 referidos a bomba 1 a bomba 8

// Blowers
//int blow[4] = {13, 14, 32, 33};  // no activos para Choele Choel, conectados en directa a 12V

const int delayTime = 350; // Tiempo de activación de la bomba

// Variables del preset
bool presetRunning = false;
unsigned long presetStartTime = 0;
unsigned long waitForReproduccionStart = 0; // Timer para espera no bloqueante antes de reproducir
const unsigned long WAIT_BEFORE_REPRODUCTION = 300000; // 5 minutos en ms (300000ms = 5min)

// Estructura para pasos de preset
struct PresetStep {
  unsigned long time;  // Tiempo en milisegundos desde el inicio
  int bombas[8];       // Estado de las bombas (1 = ON, 0 = OFF)
};

// Tiempos de activación y duración aleatorios para cada bomba
unsigned long randomStartTimes[8];
unsigned long randomDurations[8];
unsigned long randomEndTimes[8];
bool randomStates[8] = {false, false, false, false, false, false, false, false}; // Estados actuales


// Secuencia del preset
PresetStep presetSequence[] = {
  // Se acciona boton Preset:

  // Huysmans dura 07min, las bombas duran 7min = 420000ms
  {3000,   {1, 0, 1, 0, 0, 0, 0, 0}},  // 00:00
  
  {10000,  {0, 0, 0, 0, 0, 0, 0, 0}},  // 00:10
  {20000,  {0, 0, 0, 0, 0, 0, 0, 0}},  // 00:20
  {30000,  {0, 1, 0, 1, 0, 0, 0, 0}},  // 00:30
  {40000,  {0, 0, 0, 0, 0, 0, 0, 0}},  // 00:40
  {50000,  {0, 0, 0, 0, 0, 0, 0, 0}},  // 00:50
  {60000,  {0, 0, 0, 0, 1, 0, 1, 0}},  // 00:60 = 01:00

  {70000,  {0, 0, 0, 0, 0, 0, 0, 0}},  // 01:10
  {80000,  {0, 0, 0, 0, 0, 0, 0, 0}},  // 01:20
  {90000,  {0, 0, 0, 0, 0, 1, 0, 1}},  // 01:30
  {100000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 01:40
  {110000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 01:50
  {120000, {1, 0, 0, 1, 0, 0, 0, 0}},  // 01:60 = 02:00

  {130000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 02:10
  {140000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 02:20
  {150000, {0, 1, 0, 0, 1, 0, 0, 0}},  // 02:30
  {160000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 02:40
  {170000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 02:50
  {180000, {0, 0, 1, 0, 0, 0, 1, 0}},  // 02:60 = 03:00 

  {190000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 03:10
  {200000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 03:20
  {210000, {0, 1, 0, 0, 1, 0, 0, 0}},  // 03:30  fin primera vuelta
  {220000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 03:40
  {230000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 03:50
  {240000, {1, 0, 1, 0, 0, 0, 0, 0}},  // 03:60 = 04:00

  {250000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 04:10
  {260000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 04:20
  {270000, {0, 1, 0, 1, 0, 0, 0, 0}},  // 04:30
  {280000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 04:40
  {290000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 04:50
  {300000, {0, 0, 0, 0, 1, 0, 1, 0}},  // 04:60 = 05:00

  {310000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 05:10
  {320000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 05:20
  {330000, {0, 0, 0, 0, 0, 1, 0, 1}},  // 05:30
  {340000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 05:40
  {350000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 05:50
  {360000, {1, 0, 0, 1, 0, 0, 0, 0}},  // 05:60 = 06:00

  {370000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 06:10
  {380000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 06:20
  {390000, {0, 1, 0, 0, 1, 0, 0, 0}},  // 06:30
  {400000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 06:40
  {410000, {0, 0, 0, 0, 0, 0, 0, 0}},  // 06:50
  {420000, {0, 0, 1, 0, 0, 0, 1, 0}},  // 06:60 = 07:00 (420000ms)

  //Luego de finalizar secuencia hay 30seg (30000ms) (420000 + 30000 = 450000 dura todo) de silencio y reposo, para volver a iniciar desde el segundo 0 Huysmans y bombas.
};


int currentPresetIndex = 0;


void setup() {

  /////DFPLAYER se abren serial para el dfplayer/////
  #ifdef ESP32
  FPSerial.begin(9600, SERIAL_8N1, 16, 17); // Start serial communication for ESP32 with 9600 baud rate, 8 data bits, no parity, and 1 stop bit
  #else
  FPSerial.begin(9600); // Start serial communication for other boards with 9600 baud rate
  #endif
  /////DFPLAYERFIN///////////////////////////////////

  Serial.begin(115200);
  Serial.println("Cargando sistema...");

  /////DFPLAYER info para monitor serial y luego nos aseguramos pausar audio desde origen (creo no necesario, hay que probar)/////
  Serial.println(F("DFRobot DFPlayer Mini Demo")); // Print a demo start message
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)")); // Print initialization message
  
  if (!myDFPlayer.begin(FPSerial)) { // Initialize the DFPlayer Mini with the defined serial interface  , FALSE
    Serial.println(F("Unable to begin:")); // If initialization fails, print an error message
    Serial.println(F("1.Please recheck the connection!")); // Suggest rechecking the connection
    Serial.println(F("2.Please insert the SD card!")); // Suggest checking for an inserted SD card
    while(true); // Stay in an infinite loop if initialization fails
  }
  Serial.println(F("DFPlayer Mini online.")); // Print a success message if initialization succeeds

  myDFPlayer.volume(10);  // Set the DFPlayer Mini volume to 5 (max is 30)
  myDFPlayer.pause();  // Pause the track on the SD card
  /////DFPLAYERFIN////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Error con Wi-Fi, reiniciando...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }

    Serial.println("Iniciando programación " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nTerminando");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  Serial.println("Listo");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  server.begin();

  for (int i = 0; i < 8; i++) {
    pinMode(bombas[i], OUTPUT);
    digitalWrite(bombas[i], LOW);
  }

  /*for (int i = 0; i < 4; i++) {
    pinMode(blow[i], OUTPUT);
    digitalWrite(blow[i], HIGH);
  }*/

  randomSeed(analogRead(36));
}

void toggleState(int index) {
  digitalWrite(bombas[index], HIGH);
  delay(delayTime);
  digitalWrite(bombas[index], LOW);
}

void handleServer() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("Nuevo Cliente.");
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /SECUENCIA") >= 0) {
              presetRunning = true;
              presetStartTime = millis();
              currentPresetIndex = 0;
            }

            if (header.indexOf("GET /Bomba") >= 0) {
              int index = header.substring(header.indexOf("GET /Bomba") + 10, header.indexOf("GET /Bomba") + 11).toInt() - 1;
              if (index >= 0 && index < 8) toggleState(index);
            }

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>");
            client.println("html { font-family: Helvetica; font-size: 10px; text-align: center; color: #515151; margin-top: 5px;}");
            client.println(".button { border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 20px; cursor: pointer; }");
            client.println(".button.on { background-color: #DA910F; margin-top: 5px; margin-bottom: 5px; }"); //#DA910F (gris) #555555 (naranja)
            client.println(".button.off { background-color: #4CAF50; }");
            client.println(".grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; margin: 20px; }");
            client.println(".bomba-name { font-size: 26px; font-weight: bold; color: #333; margin-bottom: 10px; }");
            client.println(".by { font-style: italic; color: #8C8C8C; margin-bottom: 0px; }");
            client.println("</style></head>");
            client.println("<body><h1>OSMO 2025 ~ Palacio Libertad</h1>");

            client.println("<div class=\"grid\">");
            for (int i = 0; i < 8; i++) {
              client.println("<div>");
              client.println("<p class=\"bomba-name\">" + bombaNames[i] + "</p>"); // Aplicar clase CSS a los nombres
              client.println("<a href=\"/Bomba" + String(i + 1) + "\"><button class=\"button off\">ON</button></a>");
              client.println("</div>");
            }
            client.println("</div>");

            client.println("<div>");
            client.println("<a href=\"/SECUENCIA\"><button class=\"button on\">SECUENCIA</button></a>");
            client.println("</div>");

            client.println("<body><h3 class=\"by\">Artistas: Sebastian Tedesco & Bruno Mesz</h3>");
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Cliente desconectado.");
  }
}


void handlePreset() {
  unsigned long currentTime = millis();
  
  /*
  // Verifica si el preset está corriendo
  if (presetRunning) {
  */

  
    // Verifica si estamos dentro de la secuencia definida
    if (currentPresetIndex < sizeof(presetSequence) / sizeof(PresetStep)) {
      PresetStep currentStep = presetSequence[currentPresetIndex];

      /////DFPLAYER reproducir pasa a 1 al apretar preset/////
      reproducir = 1;
      /////DFPLAYERFIN////////////////////////////////////////

      if (currentTime - presetStartTime >= currentStep.time) {
        // Ejecuta el estado actual de las bombas
        for (int i = 0; i < 8; i++) {
          digitalWrite(bombas[i], currentStep.bombas[i] ? HIGH : LOW);
        }

        // Mantén las bombas encendidas por el tiempo definido
        delay(delayTime);

        // Apaga todas las bombas
        for (int i = 0; i < 8; i++) {
          digitalWrite(bombas[i], LOW);
        }

        // Pasa al siguiente paso
        currentPresetIndex++;
      }
    } 
    
    // A los 7min (420000ms) pausa audio
    if (currentTime - presetStartTime >= 420000 + esperaBombas) {
      myDFPlayer.pause();
    }
    
    // Verifica si han pasado los 07:00 (420000 ms)
    if (currentTime - presetStartTime >= 420000+esperaBombas) {
      
      // Apaga todas las bombas
      for (int i = 0; i < 8; i++) {
        digitalWrite(bombas[i], LOW);
      }

      // Espera 2 minutos (120000 ms) antes de reiniciar
      static unsigned long restartTimerStart = 0;  // 1380000ms serian 23min de espera

      if (restartTimerStart == 0) {
        restartTimerStart = currentTime; // Marca el inicio del temporizador
        Serial.println("Esperando 2 minutos antes de reiniciar...");
      }

      if (currentTime - restartTimerStart >= 0) { // 2 min antes de reiniciar y volver a loop
        // Reinicia los parámetros
        presetRunning = false; // Puede estar implícito si el sistema reinicia automáticamente
        currentPresetIndex = 0;
        presetStartTime = millis(); // Reinicia el tiempo de inicio
        restartTimerStart = 0; // Resetea el temporizador
        Serial.println("Reiniciando la secuencia completa...");
        
        /////DFPLAYER inicia al terminar los 8 min, osea al instante 0/////
        reproducir = 0;
        ancla = 1;
        waitForReproduccionStart = currentTime; // Inicia timer de espera no bloqueante
        /////DFPLAYERFIN///////////////////////////////////////////////////
      }
      
      // Espera no bloqueante de 5 minutos antes de activar reproducción
      if (waitForReproduccionStart > 0 && currentTime - waitForReproduccionStart >= WAIT_BEFORE_REPRODUCTION) {
        reproducir = 1;
        waitForReproduccionStart = 0; // Resetea el timer
        Serial.println("Espera completada - Reproducción activada");
      }
    /*}*/
  }
}

void loop() {
  ArduinoOTA.handle();
  handleServer();
  handlePreset();

  /////DFPLAYER preset dio 1 a reproducir y ancla originalmente es 1/////
  if (reproducir == 1 && ancla == 1){
    //delay(esperaAudio);
    myDFPlayer.volume(30);  // Elegimos volumen (maximo es 30)
    myDFPlayer.play(3);  // Inicia la reproducción del primer track de la tarjeta micro sd
    ancla = 0; // Ancla deja de ser 1, por lo que nunca más (hasta apagar y prender el dispositivo y presionar preset de nuevo) se va a leer este IF
  }
  /////DFPLAYERFIN///////////////////////////////////////////////////////
  
}

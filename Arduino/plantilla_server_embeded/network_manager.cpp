#include "network_manager.h"
#include "config.h"
#include "pump_controller.h"

// Variable estática para el callback wrapper
NetworkManager* NetworkManager::instance = nullptr;

NetworkManager::NetworkManager() : webServer(nullptr), webSocketServer(nullptr), 
                                   wifiConnected(false), serverStarted(false), lastStatusCheck(0),
                                   pumpController(nullptr) {
    instance = this;
}

NetworkManager::~NetworkManager() {
    if (webServer) {
        delete webServer;
    }
    if (webSocketServer) {
        delete webSocketServer;
    }
}

bool NetworkManager::initialize(PumpController* pumpCtrl) {
    pumpController = pumpCtrl;
    // Configurar WiFi
    WiFi.mode(WIFI_STA);
    WiFi.hostname(webServerConfig.hostname);
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    
    Serial.println("Conectando a WiFi...");
    
    // Esperar conexión WiFi
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println();
        Serial.println("WiFi conectado!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // Inicializar servidor web
        webServer = new ESP8266WebServer(webServerConfig.port);
        webSocketServer = new WebSocketsServer(81);
        
        // Configurar rutas del servidor web
        webServer->on("/", [this]() { handleRoot(); });
        webServer->on("/piano.html", [this]() { handlePiano(); });
        webServer->on("/common.css", [this]() { handleCommonCSS(); });
        webServer->on("/piano.css", [this]() { handlePianoCSS(); });
        webServer->on("/piano.js", [this]() { handlePianoJS(); });
        webServer->on("/api/status", [this]() { handleAPIStatus(); });
        webServer->on("/api/command/OSMO_PIANO_001", HTTP_POST, [this]() { handleAPICommand(); });
        webServer->onNotFound([this]() { handleNotFound(); });
        
        // Configurar WebSocket
        webSocketServer->begin();
        webSocketServer->onEvent(webSocketEventWrapper);
        
        // Iniciar servidor
        webServer->begin();
        serverStarted = true;
        
        Serial.println("Servidor web iniciado");
        Serial.print("Accede a: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/piano.html");
        
        return true;
    } else {
        Serial.println("Error: No se pudo conectar a WiFi");
        return false;
    }
}

void NetworkManager::loop() {
    if (wifiConnected && serverStarted) {
        webServer->handleClient();
        webSocketServer->loop();
        
        // Enviar estado periódicamente por WebSocket
        if (millis() - lastStatusCheck > 2000) {
            sendWebSocketMessage(getSystemStatus());
            lastStatusCheck = millis();
        }
    }
}

bool NetworkManager::isConnected() {
    return wifiConnected && WiFi.status() == WL_CONNECTED;
}

void NetworkManager::sendWebSocketMessage(const String& message) {
    if (webSocketServer) {
        String msg = message; // Crear una copia no constante
        webSocketServer->broadcastTXT(msg);
    }
}

void NetworkManager::handleRoot() {
    // Redirigir a piano.html
    webServer->sendHeader("Location", "/piano.html", true);
    webServer->send(302, "text/plain", "");
}

void NetworkManager::handlePiano() {
    String html = R"(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <title>Piano de Aromas</title>
  <link rel="stylesheet" href="/common.css" />
  <link rel="stylesheet" href="/piano.css" />
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;600&display=swap" rel="stylesheet">
  <link href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined" rel="stylesheet" />
  <script src="https://cdnjs.cloudflare.com/ajax/libs/p5.js/1.7.0/p5.min.js"></script>
</head>
<body>
  <header class="app-header">
    <div class="app-nav">
      <span class="brand">Motete Transensorial</span>
      <a href="/">Estado general</a>
      <a href="/piano.html">Piano</a>
      <span class="muted" style="margin-left:auto">Servidor: <b id="server-status">Conectado</b></span>
    </div>
  </header>
  
  <div class="container sketchContainer">
    <h2>Piano de Aromas</h2>

  <div class="controls">
    <p style="margin: 10px 0; font-size: 14px; color: #666;">
      La duración de activación se toma de la configuración de cada bomba del Osmo
    </p>
  </div>

    <div class="panel" id="piano-sketch"></div>
  </div>

  <!-- Configuración de nombres de teclas -->
  <div id="configModal" style="display:none; position:fixed; inset:0; background:rgba(0,0,0,0.45); z-index:1000;">
    <div style="position:absolute; left:50%; top:50%; transform:translate(-50%, -50%); background:#171a21; border:1px solid #2a2f3a; border-radius:12px; width: min(520px, 90vw); padding:16px; color:#e6e9ef;">
      <div style="display:flex; align-items:center; justify-content:space-between; margin-bottom:12px;">
        <h3 id="configTitle" style="margin:0; font-family: Orbitron, Segoe UI, Inter;">Configurar nombres</h3>
        <button id="configCloseBtn" class="btn" style="background:#2a2f3a;">✕</button>
      </div>
      <p class="muted" style="margin-top:0;">Asigna etiquetas a cada tecla (por ejemplo: "Lavanda", "P0", "Percusión").</p>
      <form id="configForm" style="max-height:50vh; overflow:auto; display:grid; grid-template-columns: 1fr 2fr; gap:10px;">
        <!-- Inputs generados dinámicamente -->
      </form>
      <div style="display:flex; gap:10px; justify-content:flex-end; margin-top:12px;">
        <button id="configCancelBtn" class="btn" style="background:#2a2f3a;">Cancelar</button>
        <button id="configSaveBtn" class="btn btn-success">Guardar</button>
      </div>
    </div>
  </div>

  <script src="/piano.js"></script>
</body>
</html>
)";
    webServer->send(200, "text/html", html);
}

void NetworkManager::handleCommonCSS() {
    String css = R"(
/* Common UI styles for all pages */

:root {
  --bg: #0f1115;
  --panel: #171a21;
  --muted: #aab2c0;
  --text: #e6e9ef;
  --primary: #4f8cff;
  --success: #2ecc71;
  --danger: #e74c3c;
  --warning: #f39c12;
  --border: #2a2f3a;
}

* { box-sizing: border-box; }
html, body { height: 100%; }
body {
  margin: 0;
  font-family: Inter, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif;
  background: var(--bg);
  color: var(--text);
}

.app-header {
  background: var(--panel);
  border-bottom: 1px solid var(--border);
  position: sticky;
  top: 0;
  z-index: 10;
}
.app-header .brand {
  font-weight: 600;
  font-size: 16px;
  color: var(--text);
}
.app-nav {
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 12px 18px;
}
.app-nav a {
  color: var(--muted);
  text-decoration: none;
  padding: 6px 10px;
  border-radius: 6px;
}
.app-nav a:hover { background: rgba(255,255,255,0.06); color: var(--text); }

.container { max-width: 1100px; margin: 24px auto; padding: 0 16px; }

h1, h2, h3 { margin: 12px 0; font-weight: 600; }
p { color: var(--muted); }

.panel {
  background: var(--panel);
  border: 1px solid var(--border);
  border-radius: 10px;
  padding: 16px;
}

.osmo-card { border-left: 4px solid var(--success); }

.controls { display: flex; align-items: center; gap: 12px; flex-wrap: wrap; margin: 12px 0; }
label { color: var(--muted); }

button, .btn {
  background: var(--primary);
  color: white;
  border: none;
  padding: 8px 12px;
  border-radius: 8px;
  cursor: pointer;
}
button:hover, .btn:hover { filter: brightness(1.1); }
.btn-success { background: var(--success); }
.btn-danger { background: var(--danger); }
.btn-info { background: var(--primary); }

.muted { color: var(--muted); }

/* Links as pills inside text navs */
nav a { color: var(--muted); }
nav a:hover { color: var(--text); }
)";
    webServer->send(200, "text/css", css);
}

void NetworkManager::handlePianoCSS() {
    String css = R"(
/* Heredar estilos globales del theme; sin fondo local */
body {
  font-family: 'Arial', sans-serif;
}

/* El header/nav se unifica desde common.css */

h2 {
  color: white;
  text-align: center;
  margin-bottom: 10px;
  text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
}

.container {
  max-width: 100% !important;
}
/* El estado del servidor se muestra en el header común */

.controls {
  background: rgba(255, 255, 255, 0.95);
  padding: 20px;
  border-radius: 15px;
  margin-bottom: 20px;
  border: none;
  box-shadow: 0 8px 25px rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
  text-align: center;
}

.controls input, .controls label {
  margin: 0 15px;
  font-size: 16px;
}

.controls input[type="checkbox"] {
  transform: scale(1.2);
  margin-right: 8px;
}

#piano-sketch {
  padding: 20px;
  margin: 0 auto;
}

/* Responsive design */
@media (max-width: 768px) {
  body {
    padding: 10px;
  }
  
  nav {
    padding: 10px;
  }
  
  nav a {
    display: block;
    margin: 5px 0;
    text-align: center;
  }
  
  .controls {
    padding: 15px;
  }
  
  .controls input, .controls label {
    display: block;
    margin: 10px 0;
  }
  
  .controls p {
    font-size: 12px !important;
  }
  
  #piano-sketch {
    padding: 10px;
  }
}
)";
    webServer->send(200, "text/css", css);
}

void NetworkManager::handlePianoJS() {
    // Leer el archivo piano.js y enviarlo
    // Por simplicidad, aquí incluimos el contenido directamente
    // En una implementación real, se podría leer desde SPIFFS
    String js = R"(
// Piano de Aromas - Sketch p5.js (Versión sin MQTT)
let osmos = [];
let osmoConfigs = {};
let configsLoaded = false;
let serverConnected = true;
let pianoBoards = [];
let activeKeys = new Set();
let keyAnimations = new Map();
let activeTimeouts = new Map();
let timeoutStartTimes = new Map();
let timeoutDurations = new Map();

const PIANO_CONFIG = {
  keyWidth: 80,
  keyHeight: 150,
  keySpacing: 5,
  ledSize: 20,
  boardSpacing: 30,
  colors: {
    white: [255, 255, 255],
    whitePressed: [200, 200, 200],
    border: [100, 100, 100],
    panelBg: [30, 34, 42],
    panelBorder: [42, 47, 58],
    accent: [79, 140, 255],
    ledOff: [60, 60, 60],
    ledOn: [0, 255, 0],
    ledActive: [255, 100, 0],
    osmo1: [255, 100, 100],
    osmo2: [100, 100, 255],
    osmo3: [100, 255, 100],
    osmo4: [255, 255, 100],
    osmo5: [255, 100, 255]
  }
};

function setup() {
  const canvas = createCanvas(1200, 600);
  canvas.parent('piano-sketch');
  
  loadStatus();
  loadConfigurations();
  
  setInterval(loadStatus, 2000);
  setInterval(loadConfigurations, 1000);

  try {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    const url = `${proto}://${location.hostname}:81`;
    const ws = new WebSocket(url);
    ws.onopen = () => console.log('🔌 WS conectado');
    ws.onmessage = (evt) => {
      try {
        const msg = JSON.parse(evt.data);
        if (msg.event === 'cooldowns') {
          window.__serverCooldowns = msg.payload || {};
        }
      } catch (e) {
        console.warn('WS mensaje no JSON', evt.data);
      }
    };
    ws.onclose = () => console.log('🔌 WS desconectado');
    window.__osmoWS = ws;
  } catch (e) {
    console.warn('⚠️ No se pudo iniciar WS:', e.message);
  }
}

function draw() {
  background(240);
  drawHeader();
  drawPianoBoards();
  updateAnimations();
}

function setupPianoBoards() {
  pianoBoards = [];
  
  osmos.forEach((osmo, osmoIndex) => {
    const pumpIds = osmo.pumps ? Object.keys(osmo.pumps) : [];
    const board = {
      osmoId: osmo.unit_id,
      osmoIndex: osmoIndex,
      x: 50,
      y: 140 + (osmoIndex * (PIANO_CONFIG.keyHeight + 120)),
      keys: []
    };
    
    pumpIds.forEach((pumpId, keyIndex) => {
      const key = {
        pumpId: parseInt(pumpId),
        x: board.x + (keyIndex * (PIANO_CONFIG.keyWidth + PIANO_CONFIG.keySpacing)),
        y: board.y,
        width: PIANO_CONFIG.keyWidth,
        height: PIANO_CONFIG.keyHeight,
        isPressed: false,
        lastLedState: 'off'
      };
      
      board.keys.push(key);
    });
    
    pianoBoards.push(board);
  });

  const neededHeight = 140 + (osmos.length * (PIANO_CONFIG.keyHeight + 120)) + 60;
  if (height !== neededHeight) {
    resizeCanvas(width, Math.max(neededHeight, 400));
  }
}

function drawHeader() {
  fill(50);
  textSize(18);
  textAlign(LEFT);
  text('Piano de Aromas', 20, 30);
  
  fill(serverConnected ? [0, 150, 0] : [200, 0, 0]);
  textSize(14);
  text(`Servidor: ${serverConnected ? 'Conectado' : 'Desconectado'}`, 20, 55);
  
  fill(configsLoaded ? [0, 150, 0] : [255, 165, 0]);
  textSize(12);
  text(`Config: ${configsLoaded ? 'Cargadas' : 'Cargando...'}`, 20, 75);
  
  fill(80);
  textSize(12);
  text(`Osmos conectados: ${osmos.length}`, 20, 95);
}

function drawPianoBoards() {
  pianoBoards.forEach(board => {
    drawPianoBoard(board);
  });
}

function drawPianoBoard(board) {
  const totalKeysWidth = board.keys.length * (PIANO_CONFIG.keyWidth + PIANO_CONFIG.keySpacing) - PIANO_CONFIG.keySpacing;
  const panelPadding = 20;
  const headerHeight = 40;
  const panelX = board.x - panelPadding;
  const panelY = board.y - headerHeight;
  const panelW = totalKeysWidth + panelPadding * 2;
  const panelH = PIANO_CONFIG.keyHeight + headerHeight + panelPadding;

  push();
  noStroke();
  fill(PIANO_CONFIG.colors.panelBg);
  rect(panelX, panelY, panelW, panelH, 12);
  noFill();
  stroke(PIANO_CONFIG.colors.panelBorder);
  strokeWeight(2);
  rect(panelX, panelY, panelW, panelH, 12);
  pop();

  push();
  fill(220);
  textSize(18);
  textAlign(LEFT, CENTER);
  textFont('Orbitron, Segoe UI, Inter, system-ui, -apple-system, Roboto, Helvetica, Arial');
  text(board.osmoId, panelX + 12, panelY + headerHeight / 2);
  pop();

  // Botón de configuración (Material Icon)
  const gearSize = 18;
  const gearX = panelX + panelW - 36;
  const gearY = panelY + headerHeight / 2 - gearSize / 2;
  push();
  noStroke();
  fill(PIANO_CONFIG.colors.accent[0], PIANO_CONFIG.colors.accent[1], PIANO_CONFIG.colors.accent[2]);
  textAlign(CENTER, CENTER);
  textSize(18);
  textFont('Material Symbols Outlined');
  text('settings', gearX + gearSize/2, gearY + gearSize/2 + 2);
  pop();
  // Guardar zona clicable del engranaje en el board para detectar clicks
  board.configHitbox = { x: gearX, y: gearY, w: gearSize, h: gearSize };

  // Indicador de configuración (siempre mostrar, con valores por defecto si no hay config)
  const cfgRoot = osmoConfigs[board.osmoId] || {};
  let anyCfg = null;
  
  // Intentar obtener configuración
  const cfgKeys = Object.keys(cfgRoot).filter(k => k.startsWith('pump_'));
  if (cfgKeys.length > 0) anyCfg = cfgRoot[cfgKeys[0]];
  if (!anyCfg && board.keys.length > 0) {
    const k0 = `pump_${board.keys[0].pumpId}`;
    if (cfgRoot[k0]) anyCfg = cfgRoot[k0];
  }
  
  // Usar valores por defecto si no hay configuración
  const actTime = anyCfg ? anyCfg.activationTime : 1000; // 1 segundo por defecto
  const cdTime = anyCfg ? anyCfg.cooldownTime : 3000;    // 3 segundos por defecto
  
  const actSec = Math.round(actTime / 100) / 10;
  const cdSec = Math.round(cdTime / 100) / 10;
  const yMid = panelY + headerHeight / 2;
  let cursorX = gearX - 10;

  push();
  textAlign(RIGHT, CENTER);
  fill(200);
  
  // cdSec s
  textFont('Orbitron, Segoe UI, Inter, system-ui, -apple-system, Roboto, Helvetica, Arial');
  textSize(13);
  const cdText = `${cdSec}s`;
  text(cdText, cursorX, yMid);
  const cdW = textWidth(cdText);
  cursorX -= (cdW + 8);

  // icono pause para cooldown
  textFont('Material Symbols Outlined');
  textSize(16);
  fill(PIANO_CONFIG.colors.ledActive[0], PIANO_CONFIG.colors.ledActive[1], PIANO_CONFIG.colors.ledActive[2]);
  text('pause', cursorX, yMid + 1);
  cursorX -= 22;

  // actSec s
  textFont('Orbitron, Segoe UI, Inter, system-ui, -apple-system, Roboto, Helvetica, Arial');
  textSize(13);
  fill(200);
  const actText = `${actSec}s`;
  text(actText, cursorX, yMid);
  const actW = textWidth(actText);
  cursorX -= (actW + 8);

  // icono play_arrow para activación
  textFont('Material Symbols Outlined');
  textSize(16);
  fill(PIANO_CONFIG.colors.ledOn[0], PIANO_CONFIG.colors.ledOn[1], PIANO_CONFIG.colors.ledOn[2]);
  text('play_arrow', cursorX, yMid + 1);
  pop();

  board.keys.forEach(key => {
    drawKey(key, board);
  });
}

function drawKey(key, board) {
  push();
  
  let fillColor = PIANO_CONFIG.colors.white;
  if (key.isPressed) {
    fillColor = PIANO_CONFIG.colors.whitePressed;
  }
  
  fill(fillColor);
  stroke(PIANO_CONFIG.colors.border);
  strokeWeight(2);
  rect(key.x, key.y, key.width, key.height, 8);
  
  drawLED(key, board);
  
  // Dibujar etiqueta de la bomba (usar nombre asignado si existe)
  const labels = (typeof keyLabelsByUnit !== 'undefined' && keyLabelsByUnit[board.osmoId]) ? keyLabelsByUnit[board.osmoId] : {};
  const keyLabel = labels && (labels[key.pumpId] || labels[String(key.pumpId)]) ? (labels[key.pumpId] || labels[String(key.pumpId)]) : `P${key.pumpId}`;
  fill(50);
  textSize(12);
  textAlign(CENTER);
  text(keyLabel, key.x + key.width/2, key.y + key.height - 10);
  
  pop();
}

function drawLED(key, board) {
  const ledX = key.x + key.width - PIANO_CONFIG.ledSize - 8;
  const ledY = key.y + 8;
  
  const timeoutKey = `${board.osmoId}_${key.pumpId}`;
  const hasLocalTimeout = activeTimeouts.has(timeoutKey);
  const isCoolingDown = hasLocalTimeout;
  
  let ledColor = isCoolingDown ? PIANO_CONFIG.colors.ledActive : PIANO_CONFIG.colors.ledOff;
  
  fill(ledColor);
  noStroke();
  ellipse(ledX + PIANO_CONFIG.ledSize/2, ledY + PIANO_CONFIG.ledSize/2, PIANO_CONFIG.ledSize, PIANO_CONFIG.ledSize);
  
  if (isCoolingDown) {
    let progress = getTimeoutProgress(timeoutKey);
    
    if (progress > 0) {
      push();
      noFill();
      stroke(ledColor);
      strokeWeight(2);
      
      const arcAngle = progress * TWO_PI;
      arc(ledX + PIANO_CONFIG.ledSize/2, ledY + PIANO_CONFIG.ledSize/2, 
          PIANO_CONFIG.ledSize + 6, PIANO_CONFIG.ledSize + 6, 
          -PI/2, -PI/2 + arcAngle);
      pop();
    }
  }
  
  const currentState = isCoolingDown ? 'active' : 'off';
  if (key.lastLedState !== currentState) {
    console.log(`🎨 LED ${board.osmoId} bomba ${key.pumpId} cambió de '${key.lastLedState}' a '${currentState}' [${new Date().toLocaleTimeString()}]`);
    key.lastLedState = currentState;
  }
}

function getTimeoutProgress(timeoutKey) {
  if (!activeTimeouts.has(timeoutKey)) {
    return 0;
  }
  
  const startTime = timeoutStartTimes.get(timeoutKey);
  const duration = timeoutDurations.get(timeoutKey);
  
  if (!startTime || !duration) {
    return 0;
  }
  
  const elapsed = millis() - startTime;
  const progress = Math.max(1 - (elapsed / duration), 0);
  
  return progress;
}

function updateAnimations() {
  keyAnimations.forEach((animation, key) => {
    animation.alpha -= 5;
    animation.size += 2;
    
    if (animation.alpha <= 0) {
      keyAnimations.delete(key);
    }
  });
}

function mousePressed() {
  const clickedKey = getKeyAtPosition(mouseX, mouseY);
  if (clickedKey) {
    playKey(clickedKey);
    return;
  }
  // Detectar click en botón de configuración
  for (let board of pianoBoards) {
    const hb = board.configHitbox;
    if (hb && mouseX >= hb.x && mouseX <= hb.x + hb.w && mouseY >= hb.y && mouseY <= hb.y + hb.h) {
      openConfigModal(board);
      return;
    }
  }
}

function getKeyAtPosition(x, y) {
  for (let board of pianoBoards) {
    for (let key of board.keys) {
      if (x >= key.x && x <= key.x + key.width &&
          y >= key.y && y <= key.y + key.height) {
        return { key: key, board: board };
      }
    }
  }
  return null;
}

function playKey(keyData) {
  const key = keyData.key;
  const board = keyData.board;
  
  const timeoutKey = `${board.osmoId}_${key.pumpId}`;
  const hasActiveTimeout = activeTimeouts.has(timeoutKey);
  
  if (key.ledState === 'active' || hasActiveTimeout) {
    console.log(`🚫 Bomba ${key.pumpId} de ${board.osmoId} está en cooldown - activación bloqueada [${new Date().toLocaleTimeString()}]`);
    return;
  }
  
  key.isPressed = true;
  sendCommand(board.osmoId, key.pumpId);
  
  setTimeout(() => {
    key.isPressed = false;
  }, 200);
}

async function loadConfigurations() {
  if (configsLoaded) {
    return;
  }
  
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    if (data.osmo_configs && Object.keys(data.osmo_configs).length > 0) {
      osmoConfigs = data.osmo_configs;
      configsLoaded = true;
    }
  } catch (error) {
    console.error('Error cargando configuraciones:', error);
  }
}

async function loadStatus() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    serverConnected = true;
    osmos = data.connected_osmos || [];
    window.__serverCooldowns = data.cooldowns || {};
    
    const serverStatus = document.getElementById('server-status');
    if (serverStatus) {
      serverStatus.textContent = serverConnected ? '✅ Conectado' : '❌ Desconectado';
    }
    
    setupPianoBoards();
    syncCooldownState();
    
  } catch (error) {
    console.error('Error cargando estado:', error);
    serverConnected = false;
    osmos = [];
  }
}

async function sendCommand(unitId, pumpId) {
  try {
    const duration = getPumpDuration(unitId, pumpId);
    console.log(`⏱️ Duración calculada para ${unitId} bomba ${pumpId}: ${duration}ms`);
    
    const response = await fetch(`/api/command/${unitId}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        action: 'activate_pump',
        params: { pump_id: pumpId}
      })
    });
    
    if (response.ok) {
      const responseData = await response.json();
      
      if (responseData.success) {
        console.log(`Comando enviado a ${unitId}: bomba ${pumpId} - EXITOSO`);
        activateKeyLED(unitId, pumpId, duration);
      } else {
        console.log(`Comando rechazado por ${unitId}: bomba ${pumpId} - ${responseData.message || 'Error desconocido'}`);
      }
      
    } else {
      console.error('Error al enviar comando:', response.status);
    }
  } catch (error) {
    console.error('Error de red:', error);
  }
}

function getPumpDuration(unitId, pumpId) {
  if (osmoConfigs[unitId] && osmoConfigs[unitId][`pump_${pumpId}`]) {
    const config = osmoConfigs[unitId][`pump_${pumpId}`];
    console.log(`✅ Usando configuración para ${unitId} bomba ${pumpId}: activación=${config.activationTime}ms, cooldown=${config.cooldownTime}ms`);
    const totalTime = config.activationTime + config.cooldownTime;
    console.log(`✅ Total calculado: ${totalTime}ms`);
    return totalTime;
  } else {
    console.log(`⚠️ No se encontró configuración para ${unitId} bomba ${pumpId}`);
  }
  
  const osmo = osmos.find(osmo => osmo.unit_id === unitId);
  if (osmo && osmo.pumps) {
    const pump = osmo.pumps[pumpId.toString()];
    if (pump && pump.cooldown_remaining !== undefined && pump.cooldown_remaining > 0) {
      console.log(`✅ Usando cooldown_remaining para ${unitId} bomba ${pumpId}: ${pump.cooldown_remaining}ms`);
      return pump.cooldown_remaining;
    }
  }
  
  const defaultTotal = 1000 + 3000;
  console.log(`⚠️ Usando duración por defecto para ${unitId} bomba ${pumpId}: ${defaultTotal}ms`);
  return defaultTotal;
}

function syncCooldownState() {
  console.log(`🔄 SYNC: syncCooldownState() ejecutándose [${new Date().toLocaleTimeString()}]`);
  pianoBoards.forEach(board => {
    const osmo = osmos.find(osmo => osmo.unit_id === board.osmoId);
    if (!osmo || !osmo.pumps) return;
    
    board.keys.forEach(key => {
      const pump = osmo.pumps[key.pumpId.toString()];
      const timeoutKey = `${board.osmoId}_${key.pumpId}`;
      
      if (activeTimeouts.has(timeoutKey)) {
        console.log(`🔄 SYNC: Ignorando sincronización para ${board.osmoId} bomba ${key.pumpId} - timeout activo`);
        return;
      }
      
      if (pump && pump.cooldown_remaining > 0 && key.ledState !== 'active') {
        key.ledState = 'active';
        key.cooldownStartTime = millis();
        key.cooldownDuration = pump.cooldown_remaining;
        key.ledProgress = 1;
      }
      
      if (pump && pump.cooldown_remaining === 0 && key.ledState === 'active') {
        console.log(`🔄 SYNC: ${board.osmoId} bomba ${key.pumpId} - cooldown_remaining=0 pero LED activo localmente - manteniendo estado`);
      }
    });
  });
}

function activateKeyLED(unitId, pumpId, duration) {
  const timeoutKey = `${unitId}_${pumpId}`;
  
  if (activeTimeouts.has(timeoutKey)) {
    clearTimeout(activeTimeouts.get(timeoutKey));
    timeoutStartTimes.delete(timeoutKey);
    timeoutDurations.delete(timeoutKey);
  }
  
  console.log(`🔴 LED activado para ${unitId} bomba ${pumpId}: ${duration}ms total [${new Date().toLocaleTimeString()}]`);
  
  timeoutStartTimes.set(timeoutKey, millis());
  timeoutDurations.set(timeoutKey, duration);
  
  const timeoutId = setTimeout(() => {
    activeTimeouts.delete(timeoutKey);
    timeoutStartTimes.delete(timeoutKey);
    timeoutDurations.delete(timeoutKey);
    console.log(`🟢 LED apagado para ${unitId} bomba ${pumpId} - tiempo completado [${new Date().toLocaleTimeString()}]`);
  }, duration);
  
  activeTimeouts.set(timeoutKey, timeoutId);
}

function windowResized() {
  resizeCanvas(1200, 600);
}

// ===== Configuración de nombres de teclas =====
let keyLabelsByUnit = {};

function openConfigModal(board) {
  const modal = document.getElementById('configModal');
  const form = document.getElementById('configForm');
  const title = document.getElementById('configTitle');
  if (!modal || !form) return;
  title.textContent = `Configurar nombres – ${board.osmoId}`;
  form.innerHTML = '';
  const labels = keyLabelsByUnit[board.osmoId] || {};
  board.keys.forEach(k => {
    const id = `keylabel_${board.osmoId}_${k.pumpId}`;
    const val = labels[k.pumpId] || `P${k.pumpId}`;
    form.insertAdjacentHTML('beforeend', `<label for="${id}">Tecla P${k.pumpId}</label><input id="${id}" type="text" value="${val}" />`);
  });
  modal.style.display = 'block';

  // Guardar
  const saveBtn = document.getElementById('configSaveBtn');
  const closeBtn = document.getElementById('configCloseBtn');
  const cancelBtn = document.getElementById('configCancelBtn');
  if (saveBtn) {
    saveBtn.onclick = (e) => {
      e.preventDefault();
      const newLabels = {};
      board.keys.forEach(k => {
        const id = `keylabel_${board.osmoId}_${k.pumpId}`;
        const input = document.getElementById(id);
        newLabels[k.pumpId] = (input && input.value) ? input.value : `P${k.pumpId}`;
      });
      keyLabelsByUnit[board.osmoId] = newLabels;
      modal.style.display = 'none';
      redraw && redraw();
    };
  }
  if (closeBtn) closeBtn.onclick = () => modal.style.display = 'none';
  if (cancelBtn) cancelBtn.onclick = (e) => { e.preventDefault(); modal.style.display = 'none'; };
}
)";
    webServer->send(200, "application/javascript", js);
}

void NetworkManager::handleAPIStatus() {
    String simulateParam = webServer->arg("simulate");
    bool simulate = (simulateParam == "true");
    
    DynamicJsonDocument doc(2048);
    doc["server_connected"] = true;
    doc["connected_osmos"] = JsonArray();
    
    // Crear un Osmo simulado
    JsonObject osmo = doc["connected_osmos"].createNestedObject();
    osmo["unit_id"] = deviceConfig.unitId;
    osmo["pumps"] = JsonObject();
    
    // Agregar bombas con estado real
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        JsonObject pump = osmo["pumps"].createNestedObject(String(i));
        pump["pump_id"] = i;
        
        if (pumpController) {
            PumpState state = pumpController->getPumpState(i);
            pump["cooldown_remaining"] = state.cooldownRemaining;
            pump["is_active"] = state.isActive;
        } else {
            pump["cooldown_remaining"] = 0;
            pump["is_active"] = false;
        }
    }
    
    // Configuraciones de bombas
    doc["osmo_configs"] = JsonObject();
    JsonObject configs = doc["osmo_configs"][deviceConfig.unitId];
    
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        JsonObject pumpConfig = configs.createNestedObject("pump_" + String(i));
        pumpConfig["activationTime"] = deviceConfig.pumpDefaults.activationTime;
        pumpConfig["cooldownTime"] = deviceConfig.pumpDefaults.cooldownTime;
    }
    
    // Cooldowns
    doc["cooldowns"] = JsonObject();
    
    String response;
    serializeJson(doc, response);
    
    webServer->send(200, "application/json", response);
}

void NetworkManager::handleAPICommand() {
    if (!webServer->hasArg("plain")) {
        webServer->send(400, "application/json", "{\"success\": false, \"message\": \"No body\"}");
        return;
    }
    
    String body = webServer->arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    
    String action = doc["action"];
    int pumpId = doc["params"]["pump_id"];
    
    DynamicJsonDocument response(256);
    
    if (action == "activate_pump" && pumpId >= 0 && pumpId < deviceConfig.pumpCount) {
        // Activar la bomba real usando el controlador
        if (pumpController && pumpController->activatePumpCommand(pumpId)) {
            response["success"] = true;
            response["message"] = "Bomba activada";
            response["pump_id"] = pumpId;
            Serial.printf("Comando recibido: activar bomba %d - EXITOSO\n", pumpId);
        } else {
            response["success"] = false;
            response["message"] = "Bomba no disponible (en cooldown)";
            response["pump_id"] = pumpId;
            Serial.printf("Comando recibido: activar bomba %d - RECHAZADO (cooldown)\n", pumpId);
        }
    } else {
        response["success"] = false;
        response["message"] = "Comando inválido";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    
    webServer->send(200, "application/json", responseStr);
}

void NetworkManager::handleNotFound() {
    webServer->send(404, "text/plain", "Not Found");
}

void NetworkManager::onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Desconectado!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocketServer->remoteIP(num);
                Serial.printf("[%u] Conectado desde %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] Texto recibido: %s\n", num, payload);
            break;
        default:
            break;
    }
}

void NetworkManager::webSocketEventWrapper(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (instance) {
        instance->onWebSocketEvent(num, type, payload, length);
    }
}

String NetworkManager::getSystemStatus() {
    DynamicJsonDocument doc(1024);
    doc["event"] = "status";
    doc["timestamp"] = millis();
    doc["server_connected"] = true;
    doc["wifi_connected"] = wifiConnected;
    doc["ip_address"] = WiFi.localIP().toString();
    
    String response;
    serializeJson(doc, response);
    return response;
}

String NetworkManager::getOsmoConfigs() {
    DynamicJsonDocument doc(1024);
    JsonObject configs = doc.createNestedObject();
    JsonObject osmoConfig = configs.createNestedObject(deviceConfig.unitId);
    
    for (int i = 0; i < deviceConfig.pumpCount; i++) {
        JsonObject pumpConfig = osmoConfig.createNestedObject("pump_" + String(i));
        pumpConfig["activationTime"] = deviceConfig.pumpDefaults.activationTime;
        pumpConfig["cooldownTime"] = deviceConfig.pumpDefaults.cooldownTime;
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

void NetworkManager::sendCommandResponse(const String& unitId, const String& response) {
    DynamicJsonDocument doc(512);
    doc["event"] = "command_response";
    doc["unit_id"] = unitId;
    doc["response"] = response;
    
    String message;
    serializeJson(doc, message);
    sendWebSocketMessage(message);
}

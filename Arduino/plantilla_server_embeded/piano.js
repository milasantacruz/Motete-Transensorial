// Piano de Aromas - Sketch p5.js (Versión sin MQTT)
let osmos = [];
let osmoConfigs = {}; // ✅ Configuraciones de bombas (se cargan una vez)
let configsLoaded = false; // ✅ Flag para saber si ya cargamos configuraciones
let serverConnected = true; // Siempre conectado al servidor embebido
let pianoBoards = []; // Array de pianos, uno por Osmo
let activeKeys = new Set();
let keyAnimations = new Map();
// Frontend ya no es fuente de verdad del cooldown. Se muestra LED/progreso con datos del servidor.
let activeTimeouts = new Map(); // ✅ Se mantiene para bloqueo inmediato local si se desea
let timeoutStartTimes = new Map(); // (no usado cuando hay servidor autoritativo)
let timeoutDurations = new Map(); // (no usado cuando hay servidor autoritativo)

// Configuración del piano
const PIANO_CONFIG = {
  keyWidth: 80,
  keyHeight: 150,
  keySpacing: 5,
  ledSize: 20, // Más grande para que sea más visible
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
    ledActive: [255, 100, 0], // Naranja
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
  
  // Cargar estado inicial
  loadStatus();
  
  // Cargar configuraciones por separado (solo una vez)
  loadConfigurations();
  
  // Configurar polling
  setInterval(loadStatus, 2000);
  
  // Configurar polling para configuraciones (más frecuente hasta que se carguen)
  setInterval(loadConfigurations, 1000);
  
  // Event listeners para controles (sin simulación)

  // 🚀 WS: conectar para recibir cooldowns en tiempo real
  try {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    const url = `${proto}://${location.host}`;
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
  
  // Dibujar título y estado
  drawHeader();
  
  // Dibujar pianos (uno por Osmo)
  drawPianoBoards();
  
  // Actualizar animaciones
  updateAnimations();
}

function setupPianoBoards() {
  pianoBoards = [];
  
  osmos.forEach((osmo, osmoIndex) => {
    const pumpIds = osmo.pumps ? Object.keys(osmo.pumps) : [];
    const board = {
      osmoId: osmo.unit_id,
      osmoIndex: osmoIndex,
      // Dibujar en columnas fijas y filas apiladas verticalmente
      x: 50,
      y: 140 + (osmoIndex * (PIANO_CONFIG.keyHeight + 120)),
      keys: []
    };
    
    // Crear teclas para cada bomba del Osmo
    pumpIds.forEach((pumpId, keyIndex) => {
      const key = {
        pumpId: parseInt(pumpId),
        x: board.x + (keyIndex * (PIANO_CONFIG.keyWidth + PIANO_CONFIG.keySpacing)),
        y: board.y,
        width: PIANO_CONFIG.keyWidth,
        height: PIANO_CONFIG.keyHeight,
        isPressed: false,
        lastLedState: 'off' // ✅ Solo para detectar cambios de estado
      };
      
      board.keys.push(key);
    });
    
    pianoBoards.push(board);
  });

  // Ajustar la altura del canvas para mostrar todos los pianos apilados
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
  
  // Estado del servidor
  fill(serverConnected ? [0, 150, 0] : [200, 0, 0]);
  textSize(14);
  text(`Servidor: ${serverConnected ? 'Conectado' : 'Desconectado'}`, 20, 55);
  
  // Estado de configuraciones
  fill(configsLoaded ? [0, 150, 0] : [255, 165, 0]);
  textSize(12);
  text(`Config: ${configsLoaded ? 'Cargadas' : 'Cargando...'}`, 20, 75);
  
  // Información de Osmos
  fill(80);
  textSize(12);
  text(`Osmos conectados: ${osmos.length}`, 20, 95);
}

function drawPianoBoards() {
  pianoBoards.forEach(board => {
    drawPianoBoard(board);
  });
}

function getServerCooldown(unitId, pumpId) {
  // Busca en el último payload de status cargado (osmos) los cooldowns agregados por backend
  // Ahora /api/status incluye response.cooldowns { unitId: { pumpId: { remainingMs, totalMs } } }
  if (!window.__serverCooldowns) return null;
  const unit = window.__serverCooldowns[unitId];
  if (!unit) return null;
  const entry = unit[pumpId];
  if (!entry) return null;
  return entry; // { remainingMs, totalMs }
}

function drawPianoBoard(board) {
  // Contenedor del teclado (panel)
  const totalKeysWidth = board.keys.length * (PIANO_CONFIG.keyWidth + PIANO_CONFIG.keySpacing) - PIANO_CONFIG.keySpacing;
  const panelPadding = 20;
  const headerHeight = 40; // espacio para título/indicadores
  const panelX = board.x - panelPadding;
  const panelY = board.y - headerHeight;
  const panelW = totalKeysWidth + panelPadding * 2;
  const panelH = PIANO_CONFIG.keyHeight + headerHeight + panelPadding;

  // fondo panel
  push();
  noStroke();
  fill(PIANO_CONFIG.colors.panelBg);
  rect(panelX, panelY, panelW, panelH, 12);
  // borde panel
  noFill();
  stroke(PIANO_CONFIG.colors.panelBorder);
  strokeWeight(2);
  rect(panelX, panelY, panelW, panelH, 12);
  pop();

  // Título del osmo dentro del panel
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
  // Color acento para el icono de configuración
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
  
  const actSec = Math.round(actTime / 100) / 10; // 1 decimal
  const cdSec = Math.round(cdTime / 100) / 10;
  const yMid = panelY + headerHeight / 2;
  let cursorX = gearX - 10; // arrancamos a la izquierda del icono settings

  push();
  textAlign(RIGHT, CENTER);
  fill(200);
  // Dibujar: [play_arrow][actSec s]  [pause][cdSec s] alineado a la derecha

  // 1) cdSec s (texto a la derecha)
  textFont('Orbitron, Segoe UI, Inter, system-ui, -apple-system, Roboto, Helvetica, Arial');
  textSize(13);
  const cdText = `${cdSec}s`;
  text(cdText, cursorX, yMid);
  const cdW = textWidth(cdText);
  cursorX -= (cdW + 8);

  // 2) icono pause para cooldown (naranja)
  textFont('Material Symbols Outlined');
  textSize(16);
  fill(PIANO_CONFIG.colors.ledActive[0], PIANO_CONFIG.colors.ledActive[1], PIANO_CONFIG.colors.ledActive[2]);
  text('pause', cursorX, yMid + 1);
  cursorX -= 22;

  // 3) actSec s
  textFont('Orbitron, Segoe UI, Inter, system-ui, -apple-system, Roboto, Helvetica, Arial');
  textSize(13);
  fill(200);
  const actText = `${actSec}s`;
  text(actText, cursorX, yMid);
  const actW = textWidth(actText);
  cursorX -= (actW + 8);

  // 4) icono play_arrow para activación (verde)
  textFont('Material Symbols Outlined');
  textSize(16);
  fill(PIANO_CONFIG.colors.ledOn[0], PIANO_CONFIG.colors.ledOn[1], PIANO_CONFIG.colors.ledOn[2]);
  text('play_arrow', cursorX, yMid + 1);
  pop();
  
  // Dibujar teclas
  board.keys.forEach(key => {
    drawKey(key, board);
  });
}

function drawKey(key, board) {
  push();
  
  // Color base de la tecla
  let fillColor = PIANO_CONFIG.colors.white;
  
  // Color cuando está presionada
  if (key.isPressed) {
    fillColor = PIANO_CONFIG.colors.whitePressed;
  }
  
  // Dibujar tecla
  fill(fillColor);
  stroke(PIANO_CONFIG.colors.border);
  strokeWeight(2);
  rect(key.x, key.y, key.width, key.height, 8);
  
  // (El indicador de configuración ahora se dibuja una sola vez por panel)

  
  // Dibujar LED indicador
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
  
  // ✅ LÓGICA: preferir cooldown enviado por servidor; fallback al timeout local
  const serverCd = getServerCooldown(board.osmoId, key.pumpId);
  const timeoutKey = `${board.osmoId}_${key.pumpId}`;
  const hasLocalTimeout = activeTimeouts.has(timeoutKey);
  const isCoolingDown = !!serverCd || hasLocalTimeout;
  
  // Color del LED: naranja si hay cooldown, gris si no
  let ledColor = isCoolingDown ? PIANO_CONFIG.colors.ledActive : PIANO_CONFIG.colors.ledOff;
  
  // Dibujar LED base
  fill(ledColor);
  noStroke();
  ellipse(ledX + PIANO_CONFIG.ledSize/2, ledY + PIANO_CONFIG.ledSize/2, PIANO_CONFIG.ledSize, PIANO_CONFIG.ledSize);
  
  // ✅ INDICADOR CIRCULAR DE CARGA (usar servidor si existe)
  if (isCoolingDown) {
    let progress = 0;
    if (serverCd) {
      // progress: tiempo restante / total
      progress = Math.max(0, serverCd.remainingMs / serverCd.totalMs);
    } else {
      // fallback local
      progress = getTimeoutProgress(timeoutKey);
    }
    
    if (progress > 0) {
      push();
      noFill();
      stroke(ledColor);
      strokeWeight(2);
      
      // El arco va de 0 a progress (mostrando tiempo restante)
      const arcAngle = progress * TWO_PI;
      arc(ledX + PIANO_CONFIG.ledSize/2, ledY + PIANO_CONFIG.ledSize/2, 
          PIANO_CONFIG.ledSize + 6, PIANO_CONFIG.ledSize + 6, 
          -PI/2, -PI/2 + arcAngle);
      pop();
    }
  }
  
  // Log cuando el LED cambia de color
  const currentState = isCoolingDown ? 'active' : 'off';
  if (key.lastLedState !== currentState) {
    console.log(`🎨 LED ${board.osmoId} bomba ${key.pumpId} cambió de '${key.lastLedState}' a '${currentState}' [${new Date().toLocaleTimeString()}]`);
    key.lastLedState = currentState;
  }
}

function getTimeoutProgress(timeoutKey) {
  // Si no hay timeout activo, retornar 0
  if (!activeTimeouts.has(timeoutKey)) {
    return 0;
  }
  
  const startTime = timeoutStartTimes.get(timeoutKey);
  const duration = timeoutDurations.get(timeoutKey);
  
  if (!startTime || !duration) {
    return 0;
  }
  
  const elapsed = millis() - startTime;
  const progress = Math.max(1 - (elapsed / duration), 0); // De 1 a 0 (tiempo restante)
  
  return progress;
}

function updateAnimations() {
  // ✅ LÓGICA SIMPLE: Solo animaciones de teclas presionadas
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
  
  // ✅ CORRECCIÓN: Verificar también si hay timeout activo
  const timeoutKey = `${board.osmoId}_${key.pumpId}`;
  const hasActiveTimeout = activeTimeouts.has(timeoutKey);
  
  // Verificar si la bomba está en cooldown (LED activo O timeout activo)
  if (key.ledState === 'active' || hasActiveTimeout) {
    console.log(`🚫 Bomba ${key.pumpId} de ${board.osmoId} está en cooldown - activación bloqueada [${new Date().toLocaleTimeString()}]`);
    return; // No permitir activación si está en cooldown
  }
  
  // Animación visual de presión
  key.isPressed = true;
  
  // Enviar comando
  sendCommand(board.osmoId, key.pumpId);
  
  // Feedback visual temporal
  setTimeout(() => {
    key.isPressed = false;
  }, 200);
}

async function loadConfigurations() {
  // Si ya tenemos configuraciones, no hacer nada
  if (configsLoaded) {
    return;
  }
  
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    console.log('📊 Datos recibidos del servidor:', data);
    
    if (data.osmo_configs && Object.keys(data.osmo_configs).length > 0) {
      osmoConfigs = data.osmo_configs;
      configsLoaded = true;
      console.log('✅ Configuraciones cargadas:', osmoConfigs);
    } else {
      console.log('⚠️ No se encontraron configuraciones en la respuesta');
    }
  } catch (error) {
    console.error('Error cargando configuraciones:', error);
  }
}

async function loadStatus() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    serverConnected = true; // Siempre conectado al servidor embebido
    osmos = data.connected_osmos || [];
    // Guardar cooldowns del servidor { unitId: { pumpId: { remainingMs, totalMs } } }
    window.__serverCooldowns = data.cooldowns || {};
    
    
    // Actualizar estado del servidor en el DOM
    const serverStatus = document.getElementById('server-status');
    if (serverStatus) {
      serverStatus.textContent = serverConnected ? '✅ Conectado' : '❌ Desconectado';
    }
    
    // Reconfigurar pianos
    setupPianoBoards();
    
    // Sincronizar estado de cooldown con datos del servidor
    syncCooldownState();
    
  } catch (error) {
    console.error('Error cargando estado:', error);
    serverConnected = false;
    osmos = [];
  }
}

async function sendCommand(unitId, pumpId) {
  try {
    // Obtener duración de la configuración del Osmo
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
      
      // ✅ CORRECCIÓN: Solo activar LED si el comando fue exitoso en el MCU
      if (responseData.success) {
        console.log(`Comando enviado a ${unitId}: bomba ${pumpId} - EXITOSO`);
        // Activar LED y configurar temporizador
        activateKeyLED(unitId, pumpId, duration);
      } else {
        console.log(`Comando rechazado por ${unitId}: bomba ${pumpId} - ${responseData.message || 'Error desconocido'}`);
        // No activar LED si el comando fue rechazado
      }
      
    } else {
      console.error('Error al enviar comando:', response.status);
    }
  } catch (error) {
    console.error('Error de red:', error);
  }
}

function getPumpDuration(unitId, pumpId) {
  
  // ✅ PRIORIDAD 1: Usar configuración del dispositivo si está disponible
  if (osmoConfigs[unitId] && osmoConfigs[unitId][`pump_${pumpId}`]) {
    const config = osmoConfigs[unitId][`pump_${pumpId}`];
    console.log(`✅ Usando configuración para ${unitId} bomba ${pumpId}: activación=${config.activationTime}ms, cooldown=${config.cooldownTime}ms`);
    // ✅ CORRECCIÓN: LED debe mostrar tiempo total (activación + cooldown)
    const totalTime = config.activationTime + config.cooldownTime;
    console.log(`✅ Total calculado: ${totalTime}ms`);
    return totalTime;
  } else {
    console.log(`⚠️ No se encontró configuración para ${unitId} bomba ${pumpId}`);
  }
  
  // ✅ PRIORIDAD 2: Usar cooldown_remaining del status si está disponible
  const osmo = osmos.find(osmo => osmo.unit_id === unitId);
  if (osmo && osmo.pumps) {
    const pump = osmo.pumps[pumpId.toString()];
    if (pump && pump.cooldown_remaining !== undefined && pump.cooldown_remaining > 0) {
      console.log(`✅ Usando cooldown_remaining para ${unitId} bomba ${pumpId}: ${pump.cooldown_remaining}ms`);
      return pump.cooldown_remaining;
    }
  }
  
  // ✅ PRIORIDAD 3: Usar duración por defecto (activación + cooldown)
  const defaultTotal = 1000 + 3000; // 1s activación + 3s cooldown = 4s total (igual que config.cpp)
  console.log(`⚠️ Usando duración por defecto para ${unitId} bomba ${pumpId}: ${defaultTotal}ms`);
  return defaultTotal;
}

function syncCooldownState() {
  // ✅ CORRECCIÓN: No interferir con LEDs manejados por setTimeout
  console.log(`🔄 SYNC: syncCooldownState() ejecutándose [${new Date().toLocaleTimeString()}]`);
  pianoBoards.forEach(board => {
    const osmo = osmos.find(osmo => osmo.unit_id === board.osmoId);
    if (!osmo || !osmo.pumps) return;
    
    board.keys.forEach(key => {
      const pump = osmo.pumps[key.pumpId.toString()];
      const timeoutKey = `${board.osmoId}_${key.pumpId}`;
      
      // ✅ CORRECCIÓN: Verificar timeout activo PRIMERO
      if (activeTimeouts.has(timeoutKey)) {
        console.log(`🔄 SYNC: Ignorando sincronización para ${board.osmoId} bomba ${key.pumpId} - timeout activo`);
        return; // Salir inmediatamente si hay timeout activo
      }
      
      // ✅ Solo sincronizar si NO hay timeout activo (no interferir con setTimeout)
      if (pump && pump.cooldown_remaining > 0 && key.ledState !== 'active') {
        // Si hay cooldown_remaining del servidor y no hay timeout local, sincronizar
        key.ledState = 'active';
        key.cooldownStartTime = millis();
        key.cooldownDuration = pump.cooldown_remaining;
        key.ledProgress = 1; // Comenzar con cooldown completo
      }
      
      // Si no hay cooldown_remaining del servidor y el LED está activo localmente, mantenerlo
      if (pump && pump.cooldown_remaining === 0 && key.ledState === 'active') {
        console.log(`🔄 SYNC: ${board.osmoId} bomba ${key.pumpId} - cooldown_remaining=0 pero LED activo localmente - manteniendo estado`);
      }
    });
  });
}

function activateKeyLED(unitId, pumpId, duration) {
  const timeoutKey = `${unitId}_${pumpId}`;
  
  // Limpiar timeout anterior si existe
  if (activeTimeouts.has(timeoutKey)) {
    clearTimeout(activeTimeouts.get(timeoutKey));
    timeoutStartTimes.delete(timeoutKey);
    timeoutDurations.delete(timeoutKey);
  }
  
  console.log(`🔴 LED activado para ${unitId} bomba ${pumpId}: ${duration}ms total [${new Date().toLocaleTimeString()}]`);
  
  // ✅ GUARDAR DATOS PARA EL PROGRESO CIRCULAR
  timeoutStartTimes.set(timeoutKey, millis());
  timeoutDurations.set(timeoutKey, duration);
  
  // ✅ LÓGICA SIMPLE: Solo manejar el timeout
  const timeoutId = setTimeout(() => {
    // Limpiar todos los datos del timeout
    activeTimeouts.delete(timeoutKey);
    timeoutStartTimes.delete(timeoutKey);
    timeoutDurations.delete(timeoutKey);
    console.log(`🟢 LED apagado para ${unitId} bomba ${pumpId} - tiempo completado [${new Date().toLocaleTimeString()}]`);
  }, duration);
  
  // Guardar timeout para poder limpiarlo si es necesario
  activeTimeouts.set(timeoutKey, timeoutId);
}

// Función para redimensionar el canvas si es necesario
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
      // Forzar redibujo inmediato
      redraw && redraw();
    };
  }
  if (closeBtn) closeBtn.onclick = () => modal.style.display = 'none';
  if (cancelBtn) cancelBtn.onclick = (e) => { e.preventDefault(); modal.style.display = 'none'; };
}

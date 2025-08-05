// Timeline Progress - Composición Temporal de Aromas
// Estado global según documentación

const timelineState = {
  // Configuración temporal
  totalDuration: 60,        // segundos
  currentTime: 0,           // posición actual
  isPlaying: false,         // estado reproducción
  
  // Animación del cursor
  animationId: null,        // ID para cancelar animación
  lastUpdateTime: 0,        // Timestamp del último frame
  
  // Drag & Drop
  draggedEvent: null,       // Evento siendo arrastrado
  isDragging: false,        // Estado de arrastre
  dragOffset: { x: 0, y: 0 }, // Offset del click al centro del evento
  
  // Tone.js
  toneInitialized: false,   // Estado de inicialización de Tone.js
  scheduledEvents: new Map(), // Map<eventId, Tone.js event ID> para eventos programados
  
  // Aromas configurados
  aromas: [],
  
  // Eventos en el tiempo
  events: [],
  
  // Configuración de vista
  zoom: 1.0,                // factor de zoom
  scrollX: 0,               // desplazamiento horizontal
  
  // Estado de Osmos (polling)
  connectedOsmos: [],
  usedPumps: new Map()      // osmoId -> Set<pumpId>
};

// Colores predefinidos para aromas
const aromaColors = [
  '#FF6B6B', '#4ECDC4', '#45B7D1', '#96CEB4', 
  '#FECA57', '#FF9FF3', '#54A0FF', '#5F27CD'
];

// Referencias DOM
let canvas, ctx;
let durationInput, playPauseBtn, stopBtn, scrubber;
let currentTimeSpan, totalTimeSpan, zoomLevelSpan;
let aromasContainer, addAromaBtn, simulateCheckbox;

// Inicialización
document.addEventListener('DOMContentLoaded', () => {
  initializeDOM();
  initializeCanvas();
  initializeControls();
  
  console.log('🔄 Iniciando polling de Osmos...');
  loadOsmosStatus();
  
  // Polling cada 2 segundos para actualizar Osmos
  setInterval(loadOsmosStatus, 2000);
  
  // Render inicial
  updateTimeDisplay();
  updateScrubber();
  renderTimeline();
  
  // Debug inicial
  console.log('🚀 Timeline inicializado. Para agregar aromas, activa el modo simulación si no tienes Osmos conectados.');
});

function initializeDOM() {
  // Elementos del DOM
  canvas = document.getElementById('timelineCanvas');
  ctx = canvas.getContext('2d');
  
  durationInput = document.getElementById('durationInput');
  playPauseBtn = document.getElementById('playPauseBtn');
  stopBtn = document.getElementById('stopBtn');
  scrubber = document.getElementById('scrubber');
  
  currentTimeSpan = document.getElementById('currentTime');
  totalTimeSpan = document.getElementById('totalTime');
  zoomLevelSpan = document.getElementById('zoomLevel');
  toneStatusSpan = document.getElementById('toneStatus');
  
  aromasContainer = document.getElementById('aromasContainer');
  addAromaBtn = document.getElementById('addAromaBtn');
  simulateCheckbox = document.getElementById('simulateCheckbox');
}

function initializeCanvas() {
  // Configurar canvas con devicePixelRatio para alta resolución
  const rect = canvas.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  
  ctx.scale(dpr, dpr);
  
  // Estilos CSS
  canvas.style.width = rect.width + 'px';
  canvas.style.height = rect.height + 'px';
}

function initializeControls() {
  // Duración total
  durationInput.addEventListener('change', (e) => {
    timelineState.totalDuration = Math.max(5, Math.min(300, Number(e.target.value)));
    durationInput.value = timelineState.totalDuration;
    scrubber.max = timelineState.totalDuration;
    updateTimeDisplay();
    renderTimeline();
  });
  
  // Controles de reproducción
  playPauseBtn.addEventListener('click', togglePlayback);
  stopBtn.addEventListener('click', stopPlayback);
  
  // Scrubber (barra de progreso)
  scrubber.addEventListener('input', (e) => {
    const newTime = parseFloat(e.target.value);
    timelineState.currentTime = Math.max(0, Math.min(newTime, timelineState.totalDuration));
    
    // Si Tone.js está activo, actualizar su posición también
    if (timelineState.toneInitialized) {
      Tone.Transport.seconds = timelineState.currentTime;
      
      // Si estaba reproduciendo, reprogramar eventos desde la nueva posición
      if (timelineState.isPlaying) {
        scheduleAllEvents();
      }
    }
    
    updateTimeDisplay();
    renderTimeline();
    
    // Auto-scroll al nuevo tiempo
    autoScrollToTime(timelineState.currentTime);
    
    console.log('🔍 Scrubber movido a:', formatTime(timelineState.currentTime));
  });
  
  // Doble click en scrubber para centrar timeline
  scrubber.addEventListener('dblclick', (e) => {
    autoScrollToTime(timelineState.currentTime);
    showSuccess('Timeline centrado en el cursor');
  });
  
  // Gestión de aromas
  addAromaBtn.addEventListener('click', addAroma);
  
  // Zoom
  document.getElementById('zoomInBtn').addEventListener('click', () => zoomTimeline(1.2));
  document.getElementById('zoomOutBtn').addEventListener('click', () => zoomTimeline(0.8));
  
  // Canvas eventos
  canvas.addEventListener('click', onCanvasClick);
  canvas.addEventListener('mousemove', onCanvasMouseMove);
  canvas.addEventListener('mousedown', onCanvasMouseDown);
  canvas.addEventListener('mouseup', onCanvasMouseUp);
  canvas.addEventListener('wheel', onCanvasWheel, { passive: false });
  
  // Eventos de teclado
  document.addEventListener('keydown', onKeyDown);
}

// === GESTIÓN DE OSMOS (reutilizada de ticker-tone) ===
async function loadOsmosStatus() {
  try {
    // Preservar estado antes del polling
    const savedState = preserveAromaState();
    
    const simulateParam = simulateCheckbox?.checked ? '?simulate=true' : '';
    console.log(`🔍 Polling Osmos... ${simulateParam ? '[SIMULACIÓN]' : '[REAL]'}`);
    
    const response = await fetch(`/api/status${simulateParam}`);
    const data = await response.json();
    
    console.log('📡 Respuesta del servidor:', data);
    
    const previousOsmos = timelineState.connectedOsmos.map(o => o.unit_id);
    const currentOsmos = (data.connected_osmos || []).map(o => o.unit_id);
    
    timelineState.connectedOsmos = data.connected_osmos || [];
    
    // Detectar cambios en Osmos
    const newOsmos = currentOsmos.filter(id => !previousOsmos.includes(id));
    const lostOsmos = previousOsmos.filter(id => !currentOsmos.includes(id));
    
    if (newOsmos.length > 0) {
      showSuccess(`Nuevos Osmos conectados: ${newOsmos.join(', ')}`);
    }
    
    if (lostOsmos.length > 0) {
      showWarning(`Osmos desconectados: ${lostOsmos.join(', ')}`);
      
      // Marcar aromas afectados como offline
      timelineState.aromas.forEach(aroma => {
        if (lostOsmos.includes(aroma.osmoId)) {
          aroma.active = false;
        }
      });
    }
    
    // Restaurar estado preservado
    restoreAromaState(savedState);
    
    updateAromaSelectors();
    updateAddAromaButton();
    
    // Validar configuración actual
    const issues = validateAromaConfiguration();
    if (issues.length > 0) {
      showWarning(`Advertencias: ${issues.join(', ')}`);
    }
    
  } catch (error) {
    console.error('Error cargando estado de Osmos:', error);
    showError('Error de conexión con el servidor');
  }
}

function pumpOccupied(osmoId, pumpId) {
  const set = timelineState.usedPumps.get(osmoId);
  return set ? set.has(pumpId) : false;
}

function occupyPump(osmoId, pumpId) {
  if (!timelineState.usedPumps.has(osmoId)) {
    timelineState.usedPumps.set(osmoId, new Set());
  }
  timelineState.usedPumps.get(osmoId).add(pumpId);
}

function releasePump(osmoId, pumpId) {
  const set = timelineState.usedPumps.get(osmoId);
  if (set) set.delete(pumpId);
}

// === GESTIÓN DE AROMAS ===
function addAroma() {
  console.log('🔧 addAroma() ejecutado');
  console.log('📊 Estado actual:', {
    connectedOsmos: timelineState.connectedOsmos.length,
    currentAromas: timelineState.aromas.length
  });
  
  const maxAromas = timelineState.connectedOsmos.length * 8;
  if (timelineState.aromas.length >= maxAromas) {
    showError('Límite máximo de aromas alcanzado');
    return;
  }
  
  if (timelineState.connectedOsmos.length === 0) {
    showError('No hay Osmos conectados. Activa el modo simulación para probar.');
    console.warn('⚠️ No hay Osmos conectados');
    return;
  }
  
  // Buscar primer osmo y pump libre
  let chosenOsmo = timelineState.connectedOsmos[0].unit_id;
  let chosenPump = null;
  
  for (let p = 0; p < 8; p++) {
    if (!pumpOccupied(chosenOsmo, p)) {
      chosenPump = p;
      break;
    }
  }
  
  if (chosenPump === null) {
    showError('No hay bombas libres en los Osmos disponibles');
    console.warn('⚠️ No hay bombas libres');
    return;
  }
  
  occupyPump(chosenOsmo, chosenPump);
  
  const aroma = {
    id: Date.now(),
    name: `Aroma ${timelineState.aromas.length + 1}`,
    osmoId: chosenOsmo,
    pumpId: chosenPump,
    active: true,
    color: aromaColors[timelineState.aromas.length % aromaColors.length]
  };
  
  timelineState.aromas.push(aroma);
  
  console.log('✅ Aroma agregado:', aroma);
  
  renderAromasUI();
  updateAddAromaButton();
  renderTimeline();
  
  showSuccess(`Aroma "${aroma.name}" agregado exitosamente`);
}

function removeAroma(aromaId) {
  const aromaIndex = timelineState.aromas.findIndex(a => a.id === aromaId);
  if (aromaIndex === -1) return;
  
  const aroma = timelineState.aromas[aromaIndex];
  
  // Liberar pump
  releasePump(aroma.osmoId, aroma.pumpId);
  
  // Remover aroma y sus eventos
  timelineState.aromas.splice(aromaIndex, 1);
  timelineState.events = timelineState.events.filter(e => e.aromaId !== aromaId);
  
  renderAromasUI();
  updateAddAromaButton();
  renderTimeline();
}

function renderAromasUI() {
  aromasContainer.innerHTML = '';
  
  timelineState.aromas.forEach(aroma => {
    const item = document.createElement('div');
    item.className = `aroma-item ${aroma.active ? 'active' : 'inactive'}`;
    item.style.borderLeftColor = aroma.color;
    
    // Verificar si está offline
    const isOffline = !timelineState.connectedOsmos.some(o => o.unit_id === aroma.osmoId);
    if (isOffline) {
      item.classList.add('offline');
    }
    
    item.innerHTML = `
      <input type="text" class="aroma-name-input" value="${aroma.name}" ${isOffline ? 'disabled' : ''}>
      <select class="osmo-select" ${isOffline ? 'disabled' : ''}>
        ${timelineState.connectedOsmos.map(osmo => 
          `<option value="${osmo.unit_id}" ${osmo.unit_id === aroma.osmoId ? 'selected' : ''}>
            ${osmo.unit_id}
          </option>`
        ).join('')}
      </select>
      <select class="pump-select" ${isOffline ? 'disabled' : ''}>
        ${Array.from({length: 8}, (_, i) => 
          `<option value="${i}" ${i === aroma.pumpId ? 'selected' : ''} 
           ${pumpOccupied(aroma.osmoId, i) && i !== aroma.pumpId ? 'disabled' : ''}>
            Bomba ${i}
          </option>`
        ).join('')}
      </select>
      <button class="toggle-status ${aroma.active ? 'active' : 'inactive'}" ${isOffline ? 'disabled' : ''}>
        ${aroma.active ? 'ON' : 'OFF'}
      </button>
      <span style="color: ${aroma.color}; font-size: 12px;">
        ${timelineState.events.filter(e => e.aromaId === aroma.id).length} eventos
      </span>
      <button class="delete-aroma" onclick="removeAroma(${aroma.id})">🗑️</button>
    `;
    
    // Event listeners
    const nameInput = item.querySelector('.aroma-name-input');
    const osmoSelect = item.querySelector('.osmo-select');
    const pumpSelect = item.querySelector('.pump-select');
    const toggleBtn = item.querySelector('.toggle-status');
    
    // Edición de nombre
    nameInput.addEventListener('blur', (e) => {
      const newName = e.target.value.trim();
      if (newName && newName !== aroma.name) {
        aroma.name = newName;
        console.log(`✏️ Aroma renombrado a: ${newName}`);
      } else if (!newName) {
        e.target.value = aroma.name; // Revertir si está vacío
      }
    });
    
    nameInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        e.target.blur();
      }
      if (e.key === 'Escape') {
        e.target.value = aroma.name;
        e.target.blur();
      }
    });
    
    osmoSelect.addEventListener('change', (e) => {
      const newOsmoId = e.target.value;
      
      // Encontrar primera bomba libre en el nuevo Osmo
      let availablePump = null;
      for (let p = 0; p < 8; p++) {
        if (!pumpOccupied(newOsmoId, p)) {
          availablePump = p;
          break;
        }
      }
      
      if (availablePump === null) {
        showError(`No hay bombas libres en ${newOsmoId}`);
        osmoSelect.value = aroma.osmoId; // Revertir selección
        return;
      }
      
      releasePump(aroma.osmoId, aroma.pumpId);
      aroma.osmoId = newOsmoId;
      aroma.pumpId = availablePump;
      occupyPump(aroma.osmoId, availablePump);
      
      showSuccess(`Aroma reasignado a ${newOsmoId}, bomba ${availablePump}`);
      renderAromasUI();
    });
    
    pumpSelect.addEventListener('change', (e) => {
      const newPump = Number(e.target.value);
      if (pumpOccupied(aroma.osmoId, newPump)) {
        showError(`La bomba ${newPump} ya está en uso en ${aroma.osmoId}`);
        pumpSelect.value = aroma.pumpId; // Revertir selección
        return;
      }
      releasePump(aroma.osmoId, aroma.pumpId);
      aroma.pumpId = newPump;
      occupyPump(aroma.osmoId, newPump);
      
      showSuccess(`Bomba cambiada a ${newPump}`);
    });
    
    toggleBtn.addEventListener('click', () => {
      aroma.active = !aroma.active;
      renderAromasUI();
      renderTimeline();
    });
    
    aromasContainer.appendChild(item);
  });
}

function updateAromaSelectors() {
  // Re-renderizar UI cuando cambien los Osmos disponibles
  renderAromasUI();
}

function updateAddAromaButton() {
  const maxAromas = timelineState.connectedOsmos.length * 8;
  addAromaBtn.disabled = timelineState.aromas.length >= maxAromas;
}

// === REPRODUCCIÓN (placeholder para Fase 5) ===
function togglePlayback() {
  if (timelineState.isPlaying) {
    pausePlayback();
  } else {
    startPlayback();
  }
}

async function startPlayback() {
  try {
    // Inicializar Tone.js si es necesario
    await initializeTone();
    
    timelineState.isPlaying = true;
    playPauseBtn.textContent = '⏸️ Pause';
    
    // Programar todos los eventos
    scheduleAllEvents();
    
    // Iniciar Tone.js Transport
    Tone.Transport.start();
    
    // Iniciar animación del cursor
    timelineState.lastUpdateTime = performance.now();
    timelineState.animationId = requestAnimationFrame(updatePlaybackPosition);
    
    showSuccess('▶️ Reproducción iniciada');
    console.log('▶️ Reproducción iniciada con Tone.js');
    
  } catch (error) {
    console.error('Error iniciando reproducción:', error);
    showError('Error al iniciar reproducción');
  }
}

function pausePlayback() {
  timelineState.isPlaying = false;
  playPauseBtn.textContent = '▶️ Play';
  
  // Pausar Tone.js
  if (timelineState.toneInitialized) {
    Tone.Transport.pause();
  }
  
  // Cancelar animación
  if (timelineState.animationId) {
    cancelAnimationFrame(timelineState.animationId);
    timelineState.animationId = null;
  }
  
  console.log('⏸️ Reproducción pausada');
}

function stopPlayback() {
  timelineState.isPlaying = false;
  timelineState.currentTime = 0;
  playPauseBtn.textContent = '▶️ Play';
  
  // Detener y resetear Tone.js
  if (timelineState.toneInitialized) {
    Tone.Transport.stop();
    Tone.Transport.position = 0;
    clearScheduledEvents();
  }
  
  // Cancelar animación
  if (timelineState.animationId) {
    cancelAnimationFrame(timelineState.animationId);
    timelineState.animationId = null;
  }
  
  updateTimeDisplay();
  updateScrubber();
  renderTimeline();
  
  console.log('⏹️ Reproducción detenida');
}

// === TONE.JS INTEGRATION ===
async function initializeTone() {
  if (timelineState.toneInitialized) return;
  
  try {
    // Inicializar contexto de audio (requerido para navegadores modernos)
    await Tone.start();
    
    // Configurar Transport
    Tone.Transport.bpm.value = 120; // BPM base (no afecta la reproducción temporal)
    Tone.Transport.timeSignature = [4, 4];
    
    timelineState.toneInitialized = true;
    
    // Actualizar indicador visual
    if (toneStatusSpan) {
      toneStatusSpan.textContent = '🎵 Activo';
      toneStatusSpan.classList.add('active');
    }
    
    console.log('🎵 Tone.js inicializado exitosamente');
    
  } catch (error) {
    console.error('Error inicializando Tone.js:', error);
    throw error;
  }
}

function scheduleAllEvents() {
  // Limpiar eventos previos
  clearScheduledEvents();
  
  // Programar cada evento
  timelineState.events.forEach(event => {
    const aroma = timelineState.aromas.find(a => a.id === event.aromaId);
    if (!aroma || !aroma.active) return;
    
    // Convertir tiempo a formato Tone.js
    const toneTime = `+${event.time}`;
    
    // Programar evento
    const toneEventId = Tone.Transport.schedule((time) => {
      triggerAromaEvent(aroma, event, time);
    }, toneTime);
    
    // Guardar referencia para poder cancelar después
    timelineState.scheduledEvents.set(event.id, toneEventId);
    
    console.log(`⏰ Evento programado: ${aroma.name} en ${event.time}s`);
  });
  
  console.log(`📅 Total eventos programados: ${timelineState.scheduledEvents.size}`);
}

function clearScheduledEvents() {
  // Cancelar todos los eventos programados
  timelineState.scheduledEvents.forEach((toneEventId) => {
    Tone.Transport.clear(toneEventId);
  });
  
  timelineState.scheduledEvents.clear();
  console.log('🗑️ Eventos programados limpiados');
}

async function triggerAromaEvent(aroma, event, toneTime) {
  try {
    // Obtener información del Osmo
    const osmo = timelineState.connectedOsmos.find(o => o.unit_id === aroma.osmoId);
    
    if (!osmo) {
      console.warn(`⚠️ Osmo ${aroma.osmoId} no encontrado para evento`);
      return;
    }
    
    console.log(`🚀 Ejecutando evento: ${aroma.name} (${aroma.osmoId}, bomba ${aroma.pumpId})`);
    
    // Preparar comando MQTT
    const command = {
      action: 'trigger_pump',
      params: {
        pump_id: aroma.pumpId,
        duration_ms: 500, // Duración por defecto
        intensity: 100    // Intensidad por defecto
      }
    };
    
    // Verificar modo simulación
    const isSimulation = simulateCheckbox?.checked || false;
    
    // Enviar comando
    const response = await fetch(`/api/command/${aroma.osmoId}${isSimulation ? '?simulate=true' : ''}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(command)
    });
    
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    
    const result = await response.json();
    
    // Mostrar resultado
    const modeText = result.simulated ? '[SIMULACIÓN]' : '';
    console.log(`✅ ${modeText} Comando enviado a ${aroma.osmoId}:`, result);
    
    // Feedback visual opcional (sin saturar)
    if (Math.random() < 0.3) { // Solo 30% de eventos muestran notificación
      showSuccess(`${modeText} ${aroma.name} activado`);
    }
    
  } catch (error) {
    console.error(`❌ Error ejecutando evento de ${aroma.name}:`, error);
    showError(`Error: ${aroma.name} - ${error.message}`);
  }
}

// === ANIMACIÓN DEL CURSOR ===
function updatePlaybackPosition(frameTime) {
  if (!timelineState.isPlaying) return;
  
  // Sincronizar con Tone.js Transport si está inicializado
  if (timelineState.toneInitialized && Tone.Transport.state === 'started') {
    // Obtener tiempo actual del Transport de Tone.js (más preciso)
    const toneTime = Tone.Transport.seconds;
    timelineState.currentTime = toneTime;
  } else {
    // Fallback: calcular tiempo usando requestAnimationFrame
    const deltaTime = (frameTime - timelineState.lastUpdateTime) / 1000;
    timelineState.lastUpdateTime = frameTime;
    timelineState.currentTime += deltaTime;
  }
  
  // Verificar si llegamos al final
  if (timelineState.currentTime >= timelineState.totalDuration) {
    timelineState.currentTime = timelineState.totalDuration;
    stopPlayback();
    showSuccess('Reproducción completada');
    return;
  }
  
  // Actualizar UI
  updateTimeDisplay();
  updateScrubber();
  renderTimeline();
  
  // Auto-scroll si el cursor sale de la vista
  autoScrollToPlayhead();
  
  // Continuar animación
  timelineState.animationId = requestAnimationFrame(updatePlaybackPosition);
}

function autoScrollToPlayhead() {
  autoScrollToTime(timelineState.currentTime);
}

function autoScrollToTime(targetTime) {
  const { LEFT_MARGIN, RIGHT_MARGIN } = TIMELINE_CONFIG;
  const targetX = LEFT_MARGIN + (targetTime * timelineScale) - timelineOffset;
  const viewportWidth = canvas.width - LEFT_MARGIN - RIGHT_MARGIN;
  
  // Si el tiempo está fuera de la vista, hacer scroll
  if (targetX < LEFT_MARGIN) {
    timelineOffset = targetTime * timelineScale - LEFT_MARGIN;
  } else if (targetX > canvas.width - RIGHT_MARGIN) {
    timelineOffset = (targetTime * timelineScale) - viewportWidth + RIGHT_MARGIN;
  }
  
  // Limitar scroll
  timelineOffset = Math.max(0, timelineOffset);
}

// === CONSTANTES DEL TIMELINE ===
const TIMELINE_CONFIG = {
  HEADER_HEIGHT: 40,     // Altura de la regla temporal
  ROW_HEIGHT: 60,        // Altura de cada fila de aroma
  ROW_MARGIN: 10,        // Margen entre filas
  LEFT_MARGIN: 150,      // Espacio para nombres de aromas
  RIGHT_MARGIN: 20,      // Margen derecho
  RULER_TICK_HEIGHT: 8,  // Altura de las marcas en la regla
  EVENT_HEIGHT: 40,      // Altura de los eventos
  EVENT_RADIUS: 8,       // Radio de los eventos circulares
  MIN_SCALE: 10,         // Píxeles por segundo (zoom mínimo)
  MAX_SCALE: 200,        // Píxeles por segundo (zoom máximo)
  DEFAULT_SCALE: 50      // Píxeles por segundo (zoom por defecto)
};

// Variables de visualización
let timelineScale = TIMELINE_CONFIG.DEFAULT_SCALE; // píxeles por segundo
let timelineOffset = 0; // desplazamiento horizontal en píxeles

// === RENDERIZADO DEL TIMELINE ===
function renderTimeline() {
  const ctx = canvas.getContext('2d');
  
  // Ajustar tamaño del canvas
  updateCanvasSize();
  
  // Limpiar canvas
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  
  // Renderizar componentes
  renderTimeRuler(ctx);
  renderAromaRows(ctx);
  renderEvents(ctx);
  renderPlaybackCursor(ctx);
}

function updateCanvasSize() {
  const container = canvas.parentElement;
  const rect = container.getBoundingClientRect();
  
  // Calcular altura necesaria
  const totalRows = timelineState.aromas.filter(a => a.active).length;
  const neededHeight = TIMELINE_CONFIG.HEADER_HEIGHT + 
                      (totalRows * (TIMELINE_CONFIG.ROW_HEIGHT + TIMELINE_CONFIG.ROW_MARGIN)) + 
                      50; // padding bottom
  
  // Actualizar dimensiones
  canvas.width = rect.width;
  canvas.height = Math.max(neededHeight, 300);
  canvas.style.height = canvas.height + 'px';
}

function renderTimeRuler(ctx) {
  const { HEADER_HEIGHT, LEFT_MARGIN, RIGHT_MARGIN } = TIMELINE_CONFIG;
  const timelineWidth = canvas.width - LEFT_MARGIN - RIGHT_MARGIN;
  
  // Fondo de la regla
  ctx.fillStyle = '#f8f9fa';
  ctx.fillRect(0, 0, canvas.width, HEADER_HEIGHT);
  
  // Línea divisoria
  ctx.strokeStyle = '#dee2e6';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, HEADER_HEIGHT);
  ctx.lineTo(canvas.width, HEADER_HEIGHT);
  ctx.stroke();
  
  // Calcular intervalos de tiempo para las marcas
  const startTime = Math.max(0, timelineOffset / timelineScale);
  const endTime = startTime + (timelineWidth / timelineScale);
  const interval = calculateRulerInterval(timelineScale);
  
  ctx.fillStyle = '#495057';
  ctx.font = '12px Arial';
  ctx.textAlign = 'center';
  
  // Dibujar marcas de tiempo
  for (let time = Math.ceil(startTime / interval) * interval; time <= endTime; time += interval) {
    const x = LEFT_MARGIN + (time * timelineScale) - timelineOffset;
    
    if (x >= LEFT_MARGIN && x <= canvas.width - RIGHT_MARGIN) {
      // Marca principal
      ctx.beginPath();
      ctx.moveTo(x, HEADER_HEIGHT - TIMELINE_CONFIG.RULER_TICK_HEIGHT);
      ctx.lineTo(x, HEADER_HEIGHT);
      ctx.stroke();
      
      // Etiqueta de tiempo
      const timeLabel = formatTime(time);
      ctx.fillText(timeLabel, x, HEADER_HEIGHT - 15);
    }
  }
  
  // Etiqueta de duración total
  ctx.textAlign = 'left';
  ctx.fillStyle = '#6c757d';
  ctx.fillText(`Duración: ${formatTime(timelineState.totalDuration)}`, 10, 20);
}

function renderAromaRows(ctx) {
  const { HEADER_HEIGHT, ROW_HEIGHT, ROW_MARGIN, LEFT_MARGIN } = TIMELINE_CONFIG;
  const activeAromas = timelineState.aromas.filter(a => a.active);
  
  activeAromas.forEach((aroma, index) => {
    const y = HEADER_HEIGHT + (index * (ROW_HEIGHT + ROW_MARGIN));
    
    // Fondo de la fila (alternado)
    ctx.fillStyle = index % 2 === 0 ? '#ffffff' : '#f8f9fa';
    ctx.fillRect(0, y, canvas.width, ROW_HEIGHT);
    
    // Línea divisoria
    ctx.strokeStyle = '#e9ecef';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, y + ROW_HEIGHT);
    ctx.lineTo(canvas.width, y + ROW_HEIGHT);
    ctx.stroke();
    
    // Área del nombre (lado izquierdo)
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, y, LEFT_MARGIN, ROW_HEIGHT);
    
    // Línea divisoria vertical
    ctx.beginPath();
    ctx.moveTo(LEFT_MARGIN, y);
    ctx.lineTo(LEFT_MARGIN, y + ROW_HEIGHT);
    ctx.stroke();
    
    // Nombre del aroma
    ctx.fillStyle = aroma.active ? '#495057' : '#6c757d';
    ctx.font = 'bold 14px Arial';
    ctx.textAlign = 'left';
    ctx.fillText(aroma.name, 10, y + ROW_HEIGHT/2 + 5);
    
    // Información del Osmo/bomba
    ctx.fillStyle = '#6c757d';
    ctx.font = '11px Arial';
    ctx.fillText(`${aroma.osmoId} - Bomba ${aroma.pumpId}`, 10, y + ROW_HEIGHT/2 + 20);
    
    // Indicador de estado (punto de color)
    const statusColor = getAromaStatusColor(aroma);
    ctx.fillStyle = statusColor;
    ctx.beginPath();
    ctx.arc(LEFT_MARGIN - 15, y + ROW_HEIGHT/2, 6, 0, 2 * Math.PI);
    ctx.fill();
  });
}

function renderEvents(ctx) {
  const { HEADER_HEIGHT, ROW_HEIGHT, ROW_MARGIN, LEFT_MARGIN, EVENT_RADIUS } = TIMELINE_CONFIG;
  const activeAromas = timelineState.aromas.filter(a => a.active);
  
  timelineState.events.forEach(event => {
    const aromaIndex = activeAromas.findIndex(a => a.id === event.aromaId);
    if (aromaIndex === -1) return; // Aroma no activo
    
    const x = LEFT_MARGIN + (event.time * timelineScale) - timelineOffset;
    const y = HEADER_HEIGHT + (aromaIndex * (ROW_HEIGHT + ROW_MARGIN)) + ROW_HEIGHT/2;
    
    // Solo renderizar si está visible
    if (x >= LEFT_MARGIN - EVENT_RADIUS && x <= canvas.width + EVENT_RADIUS) {
      const aroma = activeAromas[aromaIndex];
      
      // Círculo del evento
      ctx.fillStyle = getAromaStatusColor(aroma);
      ctx.beginPath();
      ctx.arc(x, y, EVENT_RADIUS, 0, 2 * Math.PI);
      ctx.fill();
      
      // Borde
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2;
      ctx.stroke();
      
      // Tiempo del evento (solo si hay espacio)
      if (timelineScale > 30) {
        ctx.fillStyle = '#495057';
        ctx.font = '10px Arial';
        ctx.textAlign = 'center';
        ctx.fillText(formatTime(event.time), x, y - 15);
      }
    }
  });
}

function renderPlaybackCursor(ctx) {
  const { HEADER_HEIGHT, LEFT_MARGIN } = TIMELINE_CONFIG;
  const x = LEFT_MARGIN + (timelineState.currentTime * timelineScale) - timelineOffset;
  
  // Solo renderizar si está visible
  if (x >= LEFT_MARGIN && x <= canvas.width) {
    // Línea del cursor
    ctx.strokeStyle = '#dc3545';
    ctx.lineWidth = 2;
    ctx.setLineDash([]);
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, canvas.height);
    ctx.stroke();
    
    // Cabeza del cursor
    ctx.fillStyle = '#dc3545';
    ctx.beginPath();
    ctx.moveTo(x, HEADER_HEIGHT);
    ctx.lineTo(x - 8, HEADER_HEIGHT - 12);
    ctx.lineTo(x + 8, HEADER_HEIGHT - 12);
    ctx.closePath();
    ctx.fill();
  }
}

// === GESTIÓN DE EVENTOS TEMPORALES ===
function addEvent(aromaId, time) {
  const newEvent = {
    id: Date.now() + Math.random(), // ID único
    aromaId: aromaId,
    time: Math.max(0, Math.min(time, timelineState.totalDuration)) // Clamp dentro de límites
  };
  
  timelineState.events.push(newEvent);
  
  // Reprogramar eventos si está reproduciendo
  if (timelineState.isPlaying && timelineState.toneInitialized) {
    scheduleAllEvents();
  }
  
  // Actualizar contador en UI de aromas
  renderAromasUI();
  renderTimeline();
  
  console.log(`✨ Evento agregado:`, newEvent);
  return newEvent;
}

function removeEvent(eventId) {
  const eventIndex = timelineState.events.findIndex(e => e.id === eventId);
  if (eventIndex !== -1) {
    const removedEvent = timelineState.events.splice(eventIndex, 1)[0];
    
    // Reprogramar eventos si está reproduciendo
    if (timelineState.isPlaying && timelineState.toneInitialized) {
      scheduleAllEvents();
    }
    
    // Actualizar contador en UI de aromas
    renderAromasUI();
    renderTimeline();
    
    console.log(`🗑️ Evento eliminado:`, removedEvent);
    return removedEvent;
  }
  return null;
}

function findEventNearClick(clickTime, aromaId) {
  const tolerance = 20 / timelineScale; // Tolerancia en segundos basada en zoom
  
  return timelineState.events.find(event => 
    event.aromaId === aromaId && 
    Math.abs(event.time - clickTime) <= tolerance
  );
}

function updateEventTime(eventId, newTime) {
  const event = timelineState.events.find(e => e.id === eventId);
  if (event) {
    event.time = Math.max(0, Math.min(newTime, timelineState.totalDuration));
    
    // Reprogramar eventos si está reproduciendo
    if (timelineState.isPlaying && timelineState.toneInitialized) {
      scheduleAllEvents();
    }
    
    renderTimeline();
    return event;
  }
  return null;
}

// === FUNCIONES AUXILIARES DEL TIMELINE ===
function calculateRulerInterval(scale) {
  // Calcular intervalo apropiado basado en el zoom
  const pixelsPerSecond = scale;
  const minPixelsBetweenMarks = 60;
  
  const intervals = [0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300];
  
  for (let interval of intervals) {
    if (interval * pixelsPerSecond >= minPixelsBetweenMarks) {
      return interval;
    }
  }
  
  return intervals[intervals.length - 1];
}

function getAromaStatusColor(aroma) {
  const isOnline = timelineState.connectedOsmos.some(o => o.unit_id === aroma.osmoId);
  if (!isOnline) return '#6c757d'; // Gris para offline
  
  // Colores por Osmo para diferenciación visual
  const colors = ['#007bff', '#28a745', '#ffc107', '#dc3545', '#6f42c1', '#fd7e14', '#20c997', '#e83e8c'];
  const osmoIndex = timelineState.connectedOsmos.findIndex(o => o.unit_id === aroma.osmoId);
  return colors[osmoIndex % colors.length];
}

function zoomTimeline(factor) {
  // Actualizar escala usando las constantes definidas
  timelineScale *= factor;
  timelineScale = Math.max(TIMELINE_CONFIG.MIN_SCALE, Math.min(TIMELINE_CONFIG.MAX_SCALE, timelineScale));
  
  // Actualizar display de zoom (convertir escala a porcentaje)
  const zoomPercent = Math.round((timelineScale / TIMELINE_CONFIG.DEFAULT_SCALE) * 100);
  zoomLevelSpan.textContent = zoomPercent + '%';
  
  renderTimeline();
}

function onCanvasMouseDown(e) {
  const rect = canvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;
  
  // Solo procesar clicks en el área del timeline
  if (x < TIMELINE_CONFIG.LEFT_MARGIN) return;
  
  // Calcular tiempo y fila
  const clickedTime = (x - TIMELINE_CONFIG.LEFT_MARGIN + timelineOffset) / timelineScale;
  const activeAromas = timelineState.aromas.filter(a => a.active);
  const clickedRowIndex = Math.floor((y - TIMELINE_CONFIG.HEADER_HEIGHT) / (TIMELINE_CONFIG.ROW_HEIGHT + TIMELINE_CONFIG.ROW_MARGIN));
  
  if (clickedRowIndex < 0 || clickedRowIndex >= activeAromas.length) return;
  
  const clickedAroma = activeAromas[clickedRowIndex];
  
  // Buscar evento existente para drag
  const eventToDrag = findEventNearClick(clickedTime, clickedAroma.id);
  
  if (eventToDrag) {
    // Iniciar drag de evento existente
    timelineState.draggedEvent = eventToDrag;
    timelineState.isDragging = true;
    
    // Calcular offset del mouse respecto al centro del evento
    const eventX = TIMELINE_CONFIG.LEFT_MARGIN + (eventToDrag.time * timelineScale) - timelineOffset;
    timelineState.dragOffset.x = x - eventX;
    
    canvas.style.cursor = 'grabbing';
    console.log(`🫳 Iniciando drag de evento:`, eventToDrag);
  }
}

function onCanvasMouseUp(e) {
  if (timelineState.isDragging && timelineState.draggedEvent) {
    // Finalizar drag
    timelineState.isDragging = false;
    timelineState.draggedEvent = null;
    canvas.style.cursor = 'pointer';
    
    showSuccess('Evento movido exitosamente');
    console.log(`🫴 Drag finalizado`);
  }
}

function onCanvasClick(e) {
  // Solo procesar clicks si no estamos en modo drag
  if (timelineState.isDragging) return;
  
  const rect = canvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;
  
  // Solo procesar clicks en el área del timeline (no en nombres)
  if (x < TIMELINE_CONFIG.LEFT_MARGIN) return;
  
  // Calcular tiempo clickeado
  const clickedTime = (x - TIMELINE_CONFIG.LEFT_MARGIN + timelineOffset) / timelineScale;
  
  // Verificar que esté dentro de la duración
  if (clickedTime < 0 || clickedTime > timelineState.totalDuration) return;
  
  // Detectar fila clickeada
  const activeAromas = timelineState.aromas.filter(a => a.active);
  const clickedRowIndex = Math.floor((y - TIMELINE_CONFIG.HEADER_HEIGHT) / (TIMELINE_CONFIG.ROW_HEIGHT + TIMELINE_CONFIG.ROW_MARGIN));
  
  if (clickedRowIndex < 0 || clickedRowIndex >= activeAromas.length) return;
  
  const clickedAroma = activeAromas[clickedRowIndex];
  
  // Verificar si hay un evento existente cerca del click
  const existingEvent = findEventNearClick(clickedTime, clickedAroma.id);
  
  if (existingEvent) {
    // Si hay un evento cerca y no era un drag, eliminarlo
    removeEvent(existingEvent.id);
    showSuccess(`Evento eliminado en ${formatTime(existingEvent.time)}`);
  } else {
    // Si no hay evento, crear uno nuevo
    addEvent(clickedAroma.id, clickedTime);
    showSuccess(`Evento agregado: ${clickedAroma.name} en ${formatTime(clickedTime)}`);
  }
}

function onCanvasMouseMove(e) {
  const rect = canvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;
  
  if (timelineState.isDragging && timelineState.draggedEvent) {
    // Mover evento durante drag
    const newTime = (x - TIMELINE_CONFIG.LEFT_MARGIN + timelineOffset - timelineState.dragOffset.x) / timelineScale;
    
    // Detectar nueva fila si el mouse se mueve verticalmente
    const activeAromas = timelineState.aromas.filter(a => a.active);
    const newRowIndex = Math.floor((y - TIMELINE_CONFIG.HEADER_HEIGHT) / (TIMELINE_CONFIG.ROW_HEIGHT + TIMELINE_CONFIG.ROW_MARGIN));
    
    if (newRowIndex >= 0 && newRowIndex < activeAromas.length) {
      const newAroma = activeAromas[newRowIndex];
      timelineState.draggedEvent.aromaId = newAroma.id;
    }
    
    // Actualizar tiempo del evento
    updateEventTime(timelineState.draggedEvent.id, newTime);
    
    // Actualizar UI de aromas si cambió
    renderAromasUI();
    
  } else {
    // Hover effects cuando no hay drag
    if (x < TIMELINE_CONFIG.LEFT_MARGIN) {
      canvas.style.cursor = 'default';
    } else {
      // Verificar si hay un evento cerca para mostrar cursor grab
      const hoverTime = (x - TIMELINE_CONFIG.LEFT_MARGIN + timelineOffset) / timelineScale;
      const activeAromas = timelineState.aromas.filter(a => a.active);
      const hoverRowIndex = Math.floor((y - TIMELINE_CONFIG.HEADER_HEIGHT) / (TIMELINE_CONFIG.ROW_HEIGHT + TIMELINE_CONFIG.ROW_MARGIN));
      
      if (hoverRowIndex >= 0 && hoverRowIndex < activeAromas.length) {
        const hoverAroma = activeAromas[hoverRowIndex];
        const nearEvent = findEventNearClick(hoverTime, hoverAroma.id);
        
        canvas.style.cursor = nearEvent ? 'grab' : 'pointer';
      } else {
        canvas.style.cursor = 'default';
      }
    }
  }
}

function onCanvasWheel(e) {
  e.preventDefault();
  
  const rect = canvas.getBoundingClientRect();
  const mouseX = e.clientX - rect.left;
  
  // Solo zoom si el mouse está en el área del timeline
  if (mouseX > TIMELINE_CONFIG.LEFT_MARGIN) {
    const zoomFactor = e.deltaY > 0 ? 0.9 : 1.1;
    
    // Calcular tiempo en la posición del mouse antes del zoom
    const timeAtMouse = (mouseX - TIMELINE_CONFIG.LEFT_MARGIN + timelineOffset) / timelineScale;
    
    // Aplicar zoom
    timelineScale *= zoomFactor;
    timelineScale = Math.max(TIMELINE_CONFIG.MIN_SCALE, Math.min(TIMELINE_CONFIG.MAX_SCALE, timelineScale));
    
    // Ajustar offset para mantener el tiempo bajo el mouse
    timelineOffset = (timeAtMouse * timelineScale) - (mouseX - TIMELINE_CONFIG.LEFT_MARGIN);
    timelineOffset = Math.max(0, timelineOffset);
    
    // Actualizar display de zoom
    const zoomPercent = Math.round((timelineScale / TIMELINE_CONFIG.DEFAULT_SCALE) * 100);
    zoomLevelSpan.textContent = zoomPercent + '%';
    
    renderTimeline();
  }
}

// === ATAJOS DE TECLADO ===
function onKeyDown(e) {
  // Espacio: Play/Pause
  if (e.code === 'Space') {
    e.preventDefault();
    togglePlayback();
    return;
  }
  
  // Escape: Cancelar drag si está activo
  if (e.code === 'Escape' && timelineState.isDragging) {
    timelineState.isDragging = false;
    timelineState.draggedEvent = null;
    canvas.style.cursor = 'pointer';
    showWarning('Arrastre cancelado');
    renderTimeline();
    return;
  }
  
  // Delete/Backspace: Eliminar todos los eventos seleccionados (placeholder para selección futura)
  if (e.code === 'Delete' || e.code === 'Backspace') {
    if (timelineState.events.length > 0) {
      const count = timelineState.events.length;
      timelineState.events = [];
      renderAromasUI();
      renderTimeline();
      showSuccess(`${count} evento(s) eliminado(s)`);
    }
    return;
  }
  
  // Home: Ir al inicio
  if (e.code === 'Home') {
    e.preventDefault();
    timelineState.currentTime = 0;
    updateTimeDisplay();
    updateScrubber();
    renderTimeline();
    autoScrollToTime(0);
    showSuccess('Ir al inicio');
    return;
  }
  
  // End: Ir al final
  if (e.code === 'End') {
    e.preventDefault();
    timelineState.currentTime = timelineState.totalDuration;
    updateTimeDisplay();
    updateScrubber();
    renderTimeline();
    autoScrollToTime(timelineState.totalDuration);
    showSuccess('Ir al final');
    return;
  }
  
  // Flecha izquierda/derecha: Navegar por pasos
  if (e.code === 'ArrowLeft' || e.code === 'ArrowRight') {
    e.preventDefault();
    const step = e.shiftKey ? 1.0 : 0.1; // Shift para pasos más grandes
    const direction = e.code === 'ArrowLeft' ? -1 : 1;
    
    timelineState.currentTime = Math.max(0, Math.min(
      timelineState.totalDuration,
      timelineState.currentTime + (step * direction)
    ));
    
    updateTimeDisplay();
    updateScrubber();
    renderTimeline();
    autoScrollToTime(timelineState.currentTime);
    return;
  }
}

// === UTILIDADES ===
function formatTime(seconds) {
  const minutes = Math.floor(seconds / 60);
  const secs = (seconds % 60).toFixed(1);
  return minutes > 0 ? `${minutes}:${secs.padStart(4, '0')}` : `${secs}s`;
}

function updateTimeDisplay() {
  currentTimeSpan.textContent = formatTime(timelineState.currentTime);
  totalTimeSpan.textContent = formatTime(timelineState.totalDuration);
}

function updateScrubber() {
  scrubber.value = timelineState.currentTime;
  scrubber.max = timelineState.totalDuration;
  scrubber.step = 0.1; // Precisión de 0.1 segundos
}

// === SISTEMA DE NOTIFICACIONES ===
function showNotification(message, type = 'info', duration = 3000) {
  const notification = document.createElement('div');
  notification.className = `notification notification-${type}`;
  notification.textContent = message;
  
  Object.assign(notification.style, {
    position: 'fixed',
    top: '20px',
    right: '20px',
    padding: '12px 20px',
    borderRadius: '6px',
    color: 'white',
    fontWeight: '500',
    zIndex: '1000',
    maxWidth: '300px',
    boxShadow: '0 4px 12px rgba(0,0,0,0.3)',
    transform: 'translateX(100%)',
    transition: 'transform 0.3s ease'
  });
  
  switch (type) {
    case 'success':
      notification.style.background = '#28a745';
      break;
    case 'error':
      notification.style.background = '#dc3545';
      break;
    case 'warning':
      notification.style.background = '#ffc107';
      notification.style.color = '#212529';
      break;
    default:
      notification.style.background = '#17a2b8';
  }
  
  document.body.appendChild(notification);
  
  // Animación de entrada
  setTimeout(() => {
    notification.style.transform = 'translateX(0)';
  }, 100);
  
  // Auto-remover
  setTimeout(() => {
    notification.style.transform = 'translateX(100%)';
    setTimeout(() => {
      if (notification.parentNode) {
        notification.parentNode.removeChild(notification);
      }
    }, 300);
  }, duration);
}

function showSuccess(message) {
  showNotification(message, 'success');
}

function showError(message) {
  showNotification(message, 'error', 4000);
}

function showWarning(message) {
  showNotification(message, 'warning');
}

// === PERSISTENCIA DE ESTADO ===
function preserveAromaState() {
  const state = {
    aromas: timelineState.aromas,
    events: timelineState.events,
    usedPumps: Array.from(timelineState.usedPumps.entries())
      .map(([osmoId, pumpSet]) => [osmoId, Array.from(pumpSet)])
  };
  return state;
}

function restoreAromaState(state) {
  if (!state) return;
  
  timelineState.aromas = state.aromas || [];
  timelineState.events = state.events || [];
  
  // Restaurar usedPumps Map
  timelineState.usedPumps.clear();
  if (state.usedPumps) {
    state.usedPumps.forEach(([osmoId, pumpArray]) => {
      timelineState.usedPumps.set(osmoId, new Set(pumpArray));
    });
  }
}

// === VALIDACIONES MEJORADAS ===
function validateAromaConfiguration() {
  const issues = [];
  
  // Verificar aromas offline
  const offlineAromas = timelineState.aromas.filter(aroma => 
    !timelineState.connectedOsmos.some(osmo => osmo.unit_id === aroma.osmoId)
  );
  
  if (offlineAromas.length > 0) {
    issues.push(`${offlineAromas.length} aroma(s) están offline`);
  }
  
  // Verificar aromas sin eventos
  const emptyAromas = timelineState.aromas.filter(aroma => 
    aroma.active && timelineState.events.filter(e => e.aromaId === aroma.id).length === 0
  );
  
  if (emptyAromas.length > 0) {
    issues.push(`${emptyAromas.length} aroma(s) activos sin eventos`);
  }
  
  return issues;
}

// Exponer funciones globales para eventos inline
window.removeAroma = removeAroma;
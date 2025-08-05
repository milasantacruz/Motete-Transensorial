// Ticker-Tone Secuenciador con detección dinámica de Osmos y gestión de filas/aromas
// Integrado con /api/status para polling de Osmos conectados

let beatsPerBar = 4;
let subdivisions = 4;
let totalSteps = beatsPerBar * subdivisions;
let connectedOsmos = [];
let rows = [];
let usedPumps = new Map(); // osmoId -> Set<pumpId>
let currentStep = 0;

// Variables de control global
let isSimulationMode = false;
let currentDuration = 1000;

// Inicializar controles
function initializeControls() {
  const durationInput = document.getElementById('durationInput');
  const simulateCheckbox = document.getElementById('simulateCheckbox');
  const startBtn = document.getElementById('startBtn');
  const bpmInput = document.getElementById('bpmInput');
  const beatsInput = document.getElementById('beatsInput');
  const subsInput = document.getElementById('subsInput');
  const applyBtn = document.getElementById('applyGridBtn');
  const addRowBtn = document.getElementById('addRowBtn');

  // Actualizar duración cuando cambie el input
  durationInput.addEventListener('change', (e) => {
    currentDuration = Number(e.target.value);
  });

  // Actualizar modo simulación
  simulateCheckbox.addEventListener('change', (e) => {
    isSimulationMode = e.target.checked;
    console.log('🎭 Modo simulación:', isSimulationMode ? 'ACTIVADO' : 'DESACTIVADO');
  });

  // Aplicar nueva configuración de compás
  applyBtn.addEventListener('click', () => {
    beatsPerBar = Math.max(1, Number(beatsInput.value) || 4);
    subdivisions = Math.max(1, Number(subsInput.value) || 4);
    totalSteps = beatsPerBar * subdivisions;
    console.log('🔄 Nuevo totalSteps:', totalSteps);
    // Ajustar longitud de patrones
    rows.forEach(r => {
      if (r.pattern.length > totalSteps) {
        r.pattern = r.pattern.slice(0, totalSteps);
      } else if (r.pattern.length < totalSteps) {
        r.pattern = r.pattern.concat(Array(totalSteps - r.pattern.length).fill(false));
      }
    });
    rebuildGrid();
  });

  // Agregar aroma (fila)
  addRowBtn.addEventListener('click', addRow);

  // Botón de inicio
  startBtn.addEventListener('click', async () => {
    await Tone.start();
    Tone.Transport.bpm.value = Number(bpmInput.value) || 90;
    Tone.Transport.start();
  });
}

// Cargar estado de Osmos desde el backend
async function loadOsmosStatus() {
  try {
    const response = await fetch('/api/status');
    const data = await response.json();
    
    console.log('📊 Datos recibidos:', data);
    
    // Actualizar lista de Osmos conectados
    connectedOsmos = data.connected_osmos || [];
    console.log('🔗 Osmos conectados:', connectedOsmos);
    
    updateAddRowButton();
    // Re-renderizar grilla para ajustar selectores, tamaño, etc.
    rebuildGrid();
    
  } catch (error) {
    console.error('Error cargando estado de Osmos:', error);
  }
}

// --------------------- GESTIÓN DE FILAS ---------------------
function pumpOccupied(osmoId, pumpId) {
  const set = usedPumps.get(osmoId);
  return set ? set.has(pumpId) : false;
}

function occupyPump(osmoId, pumpId) {
  if (!usedPumps.has(osmoId)) usedPumps.set(osmoId, new Set());
  usedPumps.get(osmoId).add(pumpId);
}

function releasePump(osmoId, pumpId) {
  const set = usedPumps.get(osmoId);
  if (set) set.delete(pumpId);
}

function updateAddRowButton() {
  const maxRows = connectedOsmos.length * 8;
  addRowBtn.disabled = rows.length >= maxRows;
}

function removeRow(rowId) {
  const idx = rows.findIndex(r => r.id === rowId);
  if (idx !== -1) {
    const row = rows[idx];
    releasePump(row.osmoId, row.pumpId);
    rows.splice(idx, 1);
    updateAddRowButton();
    rebuildGrid();
  }
}

function addRow() {
  const maxRows = connectedOsmos.length * 8;
  if (rows.length >= maxRows) {
    alert('Límite máximo de aromas alcanzado');
    return;
  }
  if (connectedOsmos.length === 0) {
    alert('No hay Osmos conectados');
    return;
  }
  // Buscar primer osmo y pump libre
  let chosenOsmo = connectedOsmos[0].unit_id;
  let chosenPump = null;
  for (let p = 0; p < 8; p++) {
    if (!pumpOccupied(chosenOsmo, p)) { chosenPump = p; break; }
  }
  if (chosenPump === null) { alert('No hay bombas libres'); return; }
  occupyPump(chosenOsmo, chosenPump);
  const row = {
    id: Date.now(),
    osmoId: chosenOsmo,
    pumpId: chosenPump,
    enabled: true,
    pattern: Array(totalSteps).fill(false)
  };
  rows.push(row);
  updateAddRowButton();
  rebuildGrid();
}

function rebuildGrid() {
  const gridEl = document.getElementById('grid');
  gridEl.innerHTML = '';
  // En lugar de calcular el tamaño, dejamos que CSS Grid lo maneje.
  // Podríamos ajustar el gap o el tamaño mínimo si fuera necesario.
  
  rows.forEach((rowObj) => {
    const rowContainer = document.createElement('div');
    const isOffline = !connectedOsmos.some(o => o.unit_id === rowObj.osmoId);
    if (isOffline) rowObj.enabled = false;
    rowContainer.className = 'row' + (isOffline ? ' offline' : '');

    // Header
    const header = document.createElement('div');
    header.className = 'row-header';
    // Osmo select
    const osmoSelect = document.createElement('select');
    connectedOsmos.forEach(o => {
      const opt = document.createElement('option');
      opt.value = o.unit_id;
      opt.textContent = o.unit_id;
      if (o.unit_id === rowObj.osmoId) opt.selected = true;
      osmoSelect.appendChild(opt);
    });
    osmoSelect.addEventListener('change', (e) => {
      releasePump(rowObj.osmoId, rowObj.pumpId);
      rowObj.osmoId = e.target.value;
      rowObj.pumpId = 0;
      occupyPump(rowObj.osmoId, 0);
      rebuildGrid();
    });
    header.appendChild(osmoSelect);

    // Pump select
    const pumpSelect = document.createElement('select');
    for (let p = 0; p < 8; p++) {
      const opt = document.createElement('option');
      opt.value = p;
      opt.textContent = p;
      if (p === rowObj.pumpId) opt.selected = true;
      if (pumpOccupied(rowObj.osmoId, p) && p !== rowObj.pumpId) opt.disabled = true;
      pumpSelect.appendChild(opt);
    }
    pumpSelect.addEventListener('change', (e) => {
      const newPump = Number(e.target.value);
      if (pumpOccupied(rowObj.osmoId, newPump)) {
        alert('Esa bomba ya está usada en otra fila');
        rebuildGrid();
        return;
      }
      releasePump(rowObj.osmoId, rowObj.pumpId);
      rowObj.pumpId = newPump;
      occupyPump(rowObj.osmoId, newPump);
    });
    if (isOffline) {
      osmoSelect.disabled = true;
      pumpSelect.disabled = true;
    }
    header.appendChild(pumpSelect);

    // Delete btn
    const deleteBtn = document.createElement('button');
    deleteBtn.textContent = '🗑️';
    deleteBtn.addEventListener('click', () => removeRow(rowObj.id));
    header.appendChild(deleteBtn);

    // Enable btn
    const toggleBtn = document.createElement('button');
    toggleBtn.textContent = rowObj.enabled ? 'Disable' : 'Enable';
    toggleBtn.className = rowObj.enabled ? 'toggle-btn' : 'toggle-btn disabled';
    toggleBtn.addEventListener('click', () => {
      rowObj.enabled = !rowObj.enabled;
      toggleBtn.textContent = rowObj.enabled ? 'Disable' : 'Enable';
      toggleBtn.classList.toggle('disabled');
    });
    header.appendChild(toggleBtn);

    rowContainer.appendChild(header);

    // Cells
    const cellsContainer = document.createElement('div');
    cellsContainer.className = 'cells-container';
    for (let step = 0; step < totalSteps; step++) {
      const cell = document.createElement('div');
      cell.classList.add('cell');
      if (rowObj.pattern[step]) cell.classList.add('active');
      cell.addEventListener('click', () => {
        rowObj.pattern[step] = !rowObj.pattern[step];
        cell.classList.toggle('active');
      });
      cellsContainer.appendChild(cell);
    }
    rowContainer.appendChild(cellsContainer);
    gridEl.appendChild(rowContainer);
  });
}

// --------------------- FIN GESTIÓN FILAS ---------------------

// Recrear la grilla basada en Osmos conectados
function recreateGrid() {
  const gridEl = document.getElementById('grid');
  console.log('🎯 Elemento grid encontrado:', gridEl);
  // Calcular tamaño de celda responsivo
  const availableWidth = window.innerWidth - 1000; // margen para header/espacios
  let cellSize = Math.floor((availableWidth / totalSteps) - 6);
  cellSize = Math.max(20, Math.min(40, cellSize));
  document.documentElement.style.setProperty('--cell-size', `${cellSize}px`);
  
  // Guardar el estado actual antes de limpiar
  const currentState = {};
  const buttonStates = {};
  
  if (matrix.length > 0) {
    matrix.forEach((row, rowIndex) => {
      row.forEach((cell, colIndex) => {
        const key = `${cell.osmoId}_${cell.step}`;
        currentState[key] = cell.active;
      });
    });
    
    // Guardar estado de botones Enable/Disable
    connectedOsmos.forEach((osmo) => {
      const toggleBtn = document.querySelector(`[data-osmo-id="${osmo.unit_id}"]`);
      if (toggleBtn) {
        buttonStates[osmo.unit_id] = toggleBtn.textContent === 'Disable';
      }
    });
  }
  
  gridEl.innerHTML = ''; // Limpiar grilla actual
  console.log('🧹 Grid limpiado');
  
  matrix = [];
  console.log('📋 Creando grilla para', connectedOsmos.length, 'Osmos');
  
  // Crear una fila por cada Osmo conectado
  connectedOsmos.forEach((osmo, rowIndex) => {
    matrix[rowIndex] = [];
    
    // Crear contenedor de fila
    const rowContainer = document.createElement('div');
    const isOffline = !connectedOsmos.some(o => o.unit_id === rowObj.osmoId);
    if (isOffline) rowObj.enabled = false;
    rowContainer.className = 'row' + (isOffline ? ' offline' : '');
    
    // Crear botón de Enable/Disable para la fila
    const rowHeader = document.createElement('div');
    rowHeader.className = 'row-header';
    
    // Restaurar estado del botón si existía
    const wasButtonEnabled = buttonStates[osmo.unit_id] || false;
    const buttonText = wasButtonEnabled ? 'Disable' : 'Enable';
    const buttonClass = wasButtonEnabled ? 'toggle-btn disabled' : 'toggle-btn';
    
    rowHeader.innerHTML = `
      <span>${osmo.unit_id}</span>
      <button class="${buttonClass}" data-osmo-id="${osmo.unit_id}">${buttonText}</button>
    `;
    rowContainer.appendChild(rowHeader);
    
    // Crear celdas para esta fila (8 bombas)
    for (let col = 0; col < totalSteps; col++) {
      const cell = document.createElement('div');
      cell.classList.add('cell');
      cell.dataset.osmoId = osmo.unit_id;
      cell.dataset.step = col;
      
      // Restaurar estado activo si existía
      const stateKey = `${osmo.unit_id}_${col}`;
      const wasActive = currentState[stateKey] || false;
      if (wasActive) {
        cell.classList.add('active');
      }
      
      rowContainer.appendChild(cell);
      
      matrix[rowIndex][col] = { 
        el: cell, 
        active: wasActive,
        osmoId: osmo.unit_id,
        step: col
      };
      
      // Event listener para activar/desactivar celda
      cell.addEventListener('click', () => {
        matrix[rowIndex][col].active = !matrix[rowIndex][col].active;
        cell.classList.toggle('active');
      });
    }
    
    // Agregar la fila al grid
    gridEl.appendChild(rowContainer);
    
    // Event listener para botón Enable/Disable
    const toggleBtn = rowHeader.querySelector('.toggle-btn');
    toggleBtn.addEventListener('click', () => {
      const isEnabled = toggleBtn.textContent === 'Enable';
      toggleBtn.textContent = isEnabled ? 'Disable' : 'Enable';
      toggleBtn.classList.toggle('disabled');
    });
  });
}

// Enviar comando a un Osmo específico
async function sendCommandToOsmo(osmoId, step, duration) {
  const simulateParam = isSimulationMode ? '?simulate=true' : '';
  
  console.log('🚀 Enviando comando:', { osmoId, step, duration, isSimulationMode, simulateParam });
  
  try {
    await fetch(`/api/command/${osmoId}${simulateParam}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ 
        action: 'aroma', 
        params: { 
          pump: step, 
          duration: duration 
        } 
      })
    });
    
    if (isSimulationMode) {
      console.log(`🎭 [SIMULACIÓN] Comando enviado a ${osmoId}: bomba ${step}, duración ${duration}ms`);
    } else {
      console.log(`📤 Comando enviado a ${osmoId}: bomba ${step}, duración ${duration}ms`);
    }
  } catch (error) {
    console.error(`Error enviando comando a ${osmoId}:`, error);
  }
}

// Loop principal de Tone.js
Tone.Transport.scheduleRepeat((time) => {
  console.log(`🎵 Tick ${currentStep + 1}/${totalSteps}`);
  
  // Recorrer todas las filas definidas
  rows.forEach(rowObj => {
    const isActive = rowObj.pattern[currentStep];
    if (!isActive) return;
    if (!rowObj.enabled) return;
    sendCommandToOsmo(rowObj.osmoId, rowObj.pumpId, currentDuration);
  });

  // Feedback visual – resaltar columna actual
  const gridEl = document.getElementById('grid');
  const cells = gridEl.querySelectorAll('.cell');
  cells.forEach((cell, idx) => {
    const stepIdx = idx % totalSteps;
    if (stepIdx === currentStep) {
      cell.classList.add('playing');
      setTimeout(() => cell.classList.remove('playing'), 100);
    }
  });
  
  currentStep = (currentStep + 1) % totalSteps;
}, "8n");

// Inicializar cuando el DOM esté listo
document.addEventListener('DOMContentLoaded', () => {
  initializeControls();
  loadOsmosStatus();
  
  // Polling cada 2 segundos para actualizar Osmos
  setInterval(loadOsmosStatus, 2000);
});
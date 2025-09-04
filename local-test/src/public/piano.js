const container = document.getElementById('osmos-container');
const simulateCheckbox = document.getElementById('simulateCheckbox');
const durationInput = document.getElementById('durationInput');

function buildUI(osmos) {
  container.innerHTML = '';
  if (osmos.length === 0) {
    container.innerHTML = '<p>Esperando Osmos...</p>';
    return;
  }

  osmos.forEach(osmo => {
    const section = document.createElement('div');
    section.className = 'osmo-section';

    // Obtener bombas disponibles del Osmo
    const pumpIds = osmo.pumps ? Object.keys(osmo.pumps) : [];
    
    section.innerHTML = `
      <h3>${osmo.unit_id}</h3>
      <p>Estado: ${osmo.status}</p>
      <div class="button-grid" id="grid-${osmo.unit_id}"></div>
    `;

    container.appendChild(section);

    const grid = section.querySelector(`#grid-${osmo.unit_id}`);
    
    // Crear botones solo para las bombas que existen
    pumpIds.forEach(pumpId => {
      const btn = document.createElement('button');
      btn.textContent = `Bomba ${pumpId}`;
      btn.className = 'piano-key';
      btn.addEventListener('click', () => {
        sendCommand(osmo.unit_id, parseInt(pumpId));
      });
      
      // Agregar botón de desactivación
      const deactivateBtn = document.createElement('button');
      deactivateBtn.textContent = `⏹️ ${pumpId}`;
      deactivateBtn.className = 'piano-key deactivate';
      deactivateBtn.addEventListener('click', () => {
        sendDeactivateCommand(osmo.unit_id, parseInt(pumpId));
      });
      
      grid.appendChild(btn);
     // grid.appendChild(deactivateBtn);
    });
  });
}

async function loadStatus() {
  const simulateParam = simulateCheckbox.checked ? '?simulate=true' : '';
  const res = await fetch(`/api/status${simulateParam}`);
  const data = await res.json();
  document.getElementById('mqtt-status').textContent = data.mqtt_connected ? '✅ Conectado' : '❌ Desconectado';
  buildUI(data.connected_osmos);
}

async function sendCommand(unitId, pumpId) {
  const simulate = simulateCheckbox.checked;
  const simulateParam = simulate ? '?simulate=true' : '';
  const duration = Number(durationInput.value) || 1000;
  
  try {
    const response = await fetch(`/api/command/${unitId}${simulateParam}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        action: 'activate_pump',
        params: { pump_id: pumpId, duration: duration }
      })
    });
    
    if (response.ok) {
      const mode = simulate ? '[SIMULACIÓN] ' : '';
      console.log(`${mode}Comando enviado a ${unitId}: bomba ${pumpId} por ${duration}ms`);
      
      // Feedback visual - buscar botón por texto
      const buttons = document.querySelectorAll('.piano-key');
      buttons.forEach(btn => {
        if (btn.textContent === `Bomba ${pumpId}`) {
          btn.style.backgroundColor = '#4CAF50';
          btn.style.transform = 'scale(0.95)';
          setTimeout(() => {
            btn.style.backgroundColor = '';
            btn.style.transform = '';
          }, 1000);
        }
      });
    } else {
      console.error('Error al enviar comando:', response.status);
    }
  } catch (error) {
    console.error('Error de red:', error);
  }
}

async function sendDeactivateCommand(unitId, pumpId) {
  const simulate = simulateCheckbox.checked;
  const simulateParam = simulate ? '?simulate=true' : '';
  
  try {
    const response = await fetch(`/api/command/${unitId}${simulateParam}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        action: 'deactivate_pump',
        params: { pump_id: pumpId }
      })
    });
    
    if (response.ok) {
      const mode = simulate ? '[SIMULACIÓN] ' : '';
      console.log(`${mode}Comando de desactivación enviado a ${unitId}: bomba ${pumpId}`);
      
      // Feedback visual
      const buttons = document.querySelectorAll('.piano-key.deactivate');
      buttons.forEach(btn => {
        if (btn.textContent === `⏹️ ${pumpId}`) {
          btn.style.backgroundColor = '#ff6b6b';
          btn.style.transform = 'scale(0.95)';
          setTimeout(() => {
            btn.style.backgroundColor = '';
            btn.style.transform = '';
          }, 1000);
        }
      });
    } else {
      console.error('Error al enviar comando de desactivación:', response.status);
    }
  } catch (error) {
    console.error('Error de red:', error);
  }
}

document.addEventListener('DOMContentLoaded', () => {
  loadStatus();
  setInterval(loadStatus, 2000);
  simulateCheckbox.addEventListener('change', loadStatus);
});
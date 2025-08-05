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

    section.innerHTML = `
      <h3>${osmo.unit_id}</h3>
      <p>Estado: ${osmo.status}</p>
      <div class="button-grid" id="grid-${osmo.unit_id}"></div>
    `;

    container.appendChild(section);

    const grid = section.querySelector(`#grid-${osmo.unit_id}`);
    for (let pump = 0; pump < 8; pump++) {
      const btn = document.createElement('button');
      btn.textContent = `Bomba ${pump}`;
      btn.addEventListener('click', () => {
        sendCommand(osmo.unit_id, pump);
      });
      grid.appendChild(btn);
    }
  });
}

async function loadStatus() {
  const simulateParam = simulateCheckbox.checked ? '?simulate=true' : '';
  const res = await fetch(`/api/status${simulateParam}`);
  const data = await res.json();
  document.getElementById('mqtt-status').textContent = data.mqtt_connected ? '✅ Conectado' : '❌ Desconectado';
  buildUI(data.connected_osmos);
}

async function sendCommand(unitId, pump) {
  const simulate = simulateCheckbox.checked;
  const simulateParam = simulate ? '?simulate=true' : '';
  const duration = Number(durationInput.value) || 1000;
  await fetch(`/api/command/${unitId}${simulateParam}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      action: 'aroma',
      params: { pump, duration }
    })
  });
  const mode = simulate ? '[SIMULACIÓN] ' : '';
 // alert(`${mode}Comando enviado a ${unitId}: bomba ${pump}`);
}

document.addEventListener('DOMContentLoaded', () => {
  loadStatus();
  setInterval(loadStatus, 2000);
  simulateCheckbox.addEventListener('change', loadStatus);
});
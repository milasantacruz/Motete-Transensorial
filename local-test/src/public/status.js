async function loadStatus() {
    const simulateCheckbox = document.getElementById('simulateCheckbox');
    const isSimulationMode = simulateCheckbox ? simulateCheckbox.checked : false;
    
    const simulateParam = isSimulationMode ? '?simulate=true' : '';
    const response = await fetch(`/api/status${simulateParam}`);
    const data = await response.json();
    
    document.getElementById('mqtt-status').innerText = data.mqtt_connected ? '✅ Conectado' : '❌ Desconectado';
    
    const container = document.getElementById('osmos-container');
    if (data.connected_osmos.length === 0) {
        container.innerHTML = '<p>Esperando Osmos...</p>';
        return;
    }

    // Generar una tarjeta por Osmo con botones dinámicos según las bombas detectadas
    container.innerHTML = data.connected_osmos.map(osmo => {
        const pumpIds = osmo.pumps ? Object.keys(osmo.pumps) : [];
        const pumpButtons = pumpIds.map(pid => `
            <button onclick="sendCommand('${osmo.unit_id}', 'aroma', { pump: ${pid}, duration: 1000 })">Activar Bomba ${pid}</button>
        `).join('');

        return `
            <div class="osmo-card">
                <h3>${osmo.unit_id}</h3>
                <p>Estado: ${osmo.status}</p>
                <p>Batería: ${osmo.battery ?? '-'}%</p>
                ${pumpButtons}
            </div>
        `;
    }).join('');
}

async function sendCommand(unitId, action, params) {
    const simulateCheckbox = document.getElementById('simulateCheckbox');
    const isSimulationMode = simulateCheckbox ? simulateCheckbox.checked : false;
    const simulateParam = isSimulationMode ? '?simulate=true' : '';
    
    await fetch(`/api/command/${unitId}${simulateParam}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action, params })
    });
    
    const mode = isSimulationMode ? '[SIMULACIÓN] ' : '';
    alert(`${mode}Comando '${action}' enviado a ${unitId}`);
}

// Inicializar polling y controles
setInterval(loadStatus, 2000);

// Event listener para el checkbox de simulación
document.addEventListener('DOMContentLoaded', () => {
    const simulateCheckbox = document.getElementById('simulateCheckbox');
    if (simulateCheckbox) {
        simulateCheckbox.addEventListener('change', () => {
            console.log('🎭 Modo simulación (Estado general):', simulateCheckbox.checked ? 'ACTIVADO' : 'DESACTIVADO');
            loadStatus(); // Recargar inmediatamente cuando cambie el checkbox
        });
    }
    loadStatus();
}); 
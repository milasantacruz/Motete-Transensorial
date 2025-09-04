async function loadStatus() {
    try {
        const simulateCheckbox = document.getElementById('simulateCheckbox');
        const isSimulationMode = simulateCheckbox ? simulateCheckbox.checked : false;
        
        const simulateParam = isSimulationMode ? '?simulate=true' : '';
        console.log('🔍 Haciendo fetch a:', `/api/status${simulateParam}`);
        
        const response = await fetch(`/api/status${simulateParam}`);
        console.log('📡 Response status:', response.status);
        console.log('📡 Response ok:', response.ok);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
                const data = await response.json();
        console.log('📊 Datos recibidos:', data);
      
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
                <div class="pump-control">
                    <button onclick="sendCommand('${osmo.unit_id}', 'activate_pump', { pump_id: ${pid}, duration: 5000 })" class="activate-btn">
                        Activar Bomba ${pid}
                    </button>
                    <button onclick="sendCommand('${osmo.unit_id}', 'deactivate_pump', { pump_id: ${pid} })" class="deactivate-btn">
                        Desactivar Bomba ${pid}
                    </button>
                    <span class="pump-status" id="pump-${osmo.unit_id}-${pid}">⚪</span>
                </div>
            `).join('');

            return `
                <div class="osmo-card">
                    <h3>${osmo.unit_id}</h3>
                    <p>Estado: ${osmo.status}</p>
                    <div class="pump-controls">
                        ${pumpButtons}
                    </div>
                    <button onclick="sendCommand('${osmo.unit_id}', 'get_status', {})" class="status-btn">
                        Obtener Estado
                    </button>
                </div>
            `;
        }).join('');
        
    } catch (error) {
        console.error('❌ Error en loadStatus:', error);
        document.getElementById('mqtt-status').innerText = '❌ Error de conexión';
        document.getElementById('osmos-container').innerHTML = `<p>Error: ${error.message}</p>`;
    }
}

async function sendCommand(unitId, action, params) {
    const simulateCheckbox = document.getElementById('simulateCheckbox');
    const isSimulationMode = simulateCheckbox ? simulateCheckbox.checked : false;
    const simulateParam = isSimulationMode ? '?simulate=true' : '';
    
    // Crear comando con estructura correcta para Arduino
    const command = {
        command_id: `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
        action: action,
        params: params,
        timestamp: Date.now()
    };
    
    console.log('📤 Enviando comando:', command);
    
    try {
        const response = await fetch(`/api/command/${unitId}${simulateParam}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(command)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const result = await response.json();
        console.log('✅ Respuesta del servidor:', result);
        
        const mode = isSimulationMode ? '[SIMULACIÓN] ' : '';
        alert(`${mode}Comando '${action}' enviado a ${unitId}\nID: ${command.command_id}`);
        
        // Actualizar estado visual de la bomba si es comando de activación/desactivación
        if (action === 'activate_pump' || action === 'deactivate_pump') {
            updatePumpStatus(unitId, params.pump_id, action);
        }
        
    } catch (error) {
        console.error('❌ Error enviando comando:', error);
        alert(`❌ Error enviando comando: ${error.message}`);
    }
}

function updatePumpStatus(unitId, pumpId, action) {
    const statusElement = document.getElementById(`pump-${unitId}-${pumpId}`);
    if (statusElement) {
        if (action === 'activate_pump') {
            statusElement.textContent = '🔴';
            statusElement.title = 'Bomba activada';
        } else if (action === 'deactivate_pump') {
            statusElement.textContent = '⚪';
            statusElement.title = 'Bomba desactivada';
        }
    }
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
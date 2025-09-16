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
        // Guardar cooldowns del servidor para reflejar estado como en Piano
        window.__serverCooldowns = data.cooldowns || {};
        
        const container = document.getElementById('osmos-container');
        if (data.connected_osmos.length === 0) {
            container.innerHTML = '<p>Esperando Osmos...</p>';
            return;
        }

        // Helper: obtener cooldown del servidor para una bomba
        function getCooldown(unitId, pumpId) {
            const cdRoot = window.__serverCooldowns || {};
            const unit = cdRoot[unitId];
            if (!unit) return null;
            // pumpId puede venir como string; las claves JSON son strings
            const entry = unit[pumpId] ?? unit[String(pumpId)] ?? unit[Number(pumpId)];
            return entry || null; // { remainingMs, totalMs }
        }

        // Generar una tarjeta por Osmo mostrando: estado, visto, configuración de bombas
        const configs = data.osmo_configs || {};
        container.innerHTML = data.connected_osmos.map(osmo => {
            const unitId = osmo.unit_id;
            const cfg = configs[unitId] || {};
            const cfgItems = Object.keys(cfg).length
              ? Object.keys(cfg).sort().map(k => {
                  const pumpId = k.replace('pump_', '');
                  const c = cfg[k];
                  return `<li>P${pumpId}: activación ${c.activationTime}ms · cooldown ${c.cooldownTime}ms</li>`;
                }).join('')
              : '<li class="muted">Sin configuración recibida</li>';

            return `
                <div class="osmo-card panel">
                    <h3>${unitId}</h3>
                    <p class="muted">Estado: ${osmo.status || 'desconocido'}${osmo.lastSeen ? ` · visto: ${new Date(osmo.lastSeen).toLocaleTimeString()}` : ''}</p>
                    <div>
                        <h4 style="margin:8px 0;">Configuración de bombas</h4>
                        <ul style="margin:0 0 8px 18px; padding:0;">
                            ${cfgItems}
                        </ul>
                    </div>
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
        console.log(`${mode}Comando '${action}' enviado a ${unitId} (ID: ${command.command_id})`);
        
        // Actualizar estado visual de la bomba si es comando de activación/desactivación
        // El estado se actualizará por polling/WS; no forzar UI aquí
        
    } catch (error) {
        console.error('❌ Error enviando comando:', error);
        alert(`❌ Error enviando comando: ${error.message}`);
    }
}

function updatePumpStatus(unitId, pumpId, action) {
    // Obsoleto: ahora el estado se refleja con cooldowns del servidor
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
    // Conectar WS para actualizar cooldowns en tiempo real como en Piano
    try {
        const proto = location.protocol === 'https:' ? 'wss' : 'ws';
        const url = `${proto}://${location.host}`;
        const ws = new WebSocket(url);
        ws.onmessage = (evt) => {
            try {
                const msg = JSON.parse(evt.data);
                if (msg.event === 'cooldowns') {
                    window.__serverCooldowns = msg.payload || {};
                    // refrescar UI sin esperar al polling
                    loadStatus();
                }
            } catch {}
        };
    } catch {}
    loadStatus();
}); 
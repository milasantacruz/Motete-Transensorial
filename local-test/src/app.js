const express = require("express");
const path = require("path");
const http = require('http');
const OsmoMQTTClient = require("./services/mqttClient");

const app = express();
const server = http.createServer(app);
let wss = null; // se inicializa luego
const PORT = 3000;

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

const mqttClient = new OsmoMQTTClient();

app.get("/api/status", (req, res) => {
  const simulate = req.query.simulate === 'true';
  console.log('📡 GET /api/status llamado, simulate:', simulate);
  
  // Si está en modo simulación, forzar que devuelva Osmos simulados
  const osmos = simulate ? mqttClient.getSimulatedOsmos() : mqttClient.getConnectedOsmos();
  
  // Obtener configuraciones de bombas
  const osmoConfigs = simulate ? {} : mqttClient.getOsmoConfigs();
  console.log('📊 Configuraciones obtenidas del mqttClient:', osmoConfigs);
  const cooldowns = simulate ? {} : mqttClient.getCooldownsSnapshot();
  
  const response = {
    mqtt_connected: mqttClient.isConnectionHealthy(),
    connected_osmos: osmos,
    osmo_configs: osmoConfigs, // ✅ Agregado: configuraciones de bombas
    cooldowns // ✅ Cooldowns autoritativos del servidor { unitId: { pumpId: { remainingMs, totalMs } } }
  };
  
  console.log('📊 Respuesta /api/status:', response);
  res.json(response);
});

app.post("/api/command/:unitId", (req, res) => {
  try {
    const { unitId } = req.params;
    const command = req.body;  // ✅ Recibir comando completo
    const simulate = req.query.simulate === 'true';
    
    console.log(`📤 Comando recibido para ${unitId}:`, command);
    
    // Validar estructura del comando
    if (!command.action) {
      return res.status(400).json({ 
        success: false, 
        error: "Comando debe incluir 'action'" 
      });
    }
    
    const commandId = mqttClient.sendCommand(unitId, command.action, command.params, simulate);
    res.json({ 
      success: true, 
      command_id: commandId, 
      simulated: simulate,
      message: `Comando '${command.action}' enviado a ${unitId}`
    });
  } catch (error) {
    console.error(`❌ Error procesando comando para ${req.params.unitId}:`, error);
    res.status(500).json({ 
      success: false, 
      error: error.message 
    });
  }
});

function broadcast(event, payload) {
  if (!wss) return;
  const msg = JSON.stringify({ event, payload });
  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(msg);
    }
  });
}

async function startServer() {
  try {
    await mqttClient.connect();
    // WebSocket Server
    const { WebSocketServer } = require('ws');
    wss = new WebSocketServer({ server });
    wss.on('connection', (ws) => {
      console.log('🔌 WS cliente conectado');
      // Enviar snapshot inicial de cooldowns
      ws.send(JSON.stringify({ event: 'cooldowns', payload: mqttClient.getCooldownsSnapshot() }));
    });

    // Hook: cuando comienzan cooldowns, emitir a los clientes
    mqttClient.onCooldownsChanged = () => {
      broadcast('cooldowns', mqttClient.getCooldownsSnapshot());
    };

    server.listen(PORT, () => {
      console.log(`🌐 Servidor web + WS en http://localhost:${PORT}`);
    });
  } catch (error) {
    console.error("❌ No se pudo iniciar el servidor:", error);
    process.exit(1);
  }
}

startServer();

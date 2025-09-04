const express = require("express");
const path = require("path");
const OsmoMQTTClient = require("./services/mqttClient");

const app = express();
const PORT = 3000;

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

const mqttClient = new OsmoMQTTClient();

app.get("/api/status", (req, res) => {
  const simulate = req.query.simulate === 'true';
  console.log('📡 GET /api/status llamado, simulate:', simulate);
  
  // Si está en modo simulación, forzar que devuelva Osmos simulados
  const osmos = simulate ? mqttClient.getSimulatedOsmos() : mqttClient.getConnectedOsmos();
  
  const response = {
    mqtt_connected: mqttClient.isConnectionHealthy(),
    connected_osmos: osmos,
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

async function startServer() {
  try {
    await mqttClient.connect();
    app.listen(PORT, () => {
      console.log(`🌐 Servidor web escuchando en http://localhost:${PORT}`);
    });
  } catch (error) {
    console.error("❌ No se pudo iniciar el servidor:", error);
    process.exit(1);
  }
}

startServer();

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
  
  // Si está en modo simulación, forzar que devuelva Osmos simulados
  const osmos = simulate ? mqttClient.getSimulatedOsmos() : mqttClient.getConnectedOsmos();
  
  res.json({
    mqtt_connected: mqttClient.isConnectionHealthy(),
    connected_osmos: osmos,
  });
});

app.post("/api/command/:unitId", (req, res) => {
  try {
    const { unitId } = req.params;
    const { action, params } = req.body;
    const simulate = req.query.simulate === 'true';
    
    const commandId = mqttClient.sendCommand(unitId, action, params, simulate);
    res.json({ success: true, command_id: commandId, simulated: simulate });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
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

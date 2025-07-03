const express = require("express");
const path = require("path");
const OsmoMQTTClient = require("./services/mqttClient");

const app = express();
const PORT = 3000;

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

const mqttClient = new OsmoMQTTClient();

app.get("/api/status", (req, res) => {
  res.json({
    mqtt_connected: mqttClient.isConnected,
    connected_osmos: mqttClient.getConnectedOsmos(),
  });
});

app.post("/api/command/:unitId", (req, res) => {
  try {
    const { unitId } = req.params;
    const { action, params } = req.body;
    const commandId = mqttClient.sendCommand(unitId, action, params);
    res.json({ success: true, command_id: commandId });
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

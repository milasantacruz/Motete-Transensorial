const mqtt = require('mqtt');
const { v4: uuidv4 } = require('uuid');

class OsmoMQTTClient {
  constructor() {
    this.client = null;
    this.connectedOsmos = new Map();
    this.isConnected = false;
  }

  async connect() {
    return new Promise((resolve, reject) => {
      this.client = mqtt.connect('mqtt://localhost:1883', {
        username: 'director',
        password: 'director', 
        clientId: 'director_' + Math.random().toString(16).substr(2, 8),
      });

      this.client.on('connect', () => {
        console.log('✅ Director conectado al broker MQTT');
        this.isConnected = true;
        this.subscribeToTopics();
        resolve();
      });

      this.client.on('error', (error) => {
        console.error('❌ Error de conexión MQTT:', error);
        reject(error);
      });

      this.client.on('message', (topic, message) => {
        this.handleMessage(topic, message);
      });
    });
  }

  subscribeToTopics() {
    const topics = [
      'motete/osmo/+/status',
      'motete/osmo/discovery'
    ];

    topics.forEach(topic => {
      this.client.subscribe(topic, { qos: 1 });
      console.log(`📡 Suscrito a: ${topic}`);
    });
  }

  handleMessage(topic, message) {
    try {
      const data = JSON.parse(message.toString());
      console.log(`📩 Mensaje recibido en ${topic}:`, data);

      if (topic.includes('/status')) {
        const unitId = topic.split('/')[2];
        this.connectedOsmos.set(unitId, {
          ...data,
          lastSeen: new Date()
        });
        console.log(`💚 Estado actualizado para ${unitId}`);
      }
    } catch (error) {
      console.error('❌ Error procesando mensaje:', error);
    }
  }

  sendCommand(unitId, action, params) {
    if (!this.isConnected) throw new Error('MQTT no conectado');
    const command = {
      timestamp: new Date().toISOString(),
      command_id: uuidv4(),
      action: action,
      params: params
    };
    const topic = `motete/director/commands/${unitId}`;
    this.client.publish(topic, JSON.stringify(command), { qos: 1 });
    console.log(`📤 Comando enviado a ${unitId}:`, command);
    return command.command_id;
  }

  getConnectedOsmos() {
    return Array.from(this.connectedOsmos.values());
  }
}

module.exports = OsmoMQTTClient;
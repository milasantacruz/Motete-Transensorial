const mqtt = require('mqtt');
const { v4: uuidv4 } = require('uuid');

class OsmoMQTTClient {
  constructor(password) {
    this.client = null;
    this.connectedOsmos = new Map();
    this.isConnected = false;
    this.password = password || 'director'; // Fallback por si no se provee
  }

  async connect() {
    return new Promise((resolve, reject) => {
      this.client = mqtt.connect('mqtt://localhost:1883', {
        username: 'director',
        password: this.password, 
        clientId: 'director_' + Math.random().toString(16).substr(2, 8),
        connectTimeout: 5000, // 5 segundos de timeout
        reconnectPeriod: 0, // No reconectar automáticamente
      });

      this.client.on('connect', () => {
        console.log('✅ Director conectado al broker MQTT');
        this.isConnected = true;
        this.subscribeToTopics();
        resolve();
      });

      this.client.on('error', (error) => {
        console.error('❌ Error de conexión MQTT:', error);
        this.isConnected = false;
        reject(error);
      });

      this.client.on('close', () => {
        console.log('🔌 Conexión MQTT cerrada');
        this.isConnected = false;
      });

      this.client.on('offline', () => {
        console.log('📴 Cliente MQTT offline');
        this.isConnected = false;
      });

      this.client.on('message', (topic, message) => {
        this.handleMessage(topic, message);
      });
    });
  }

  subscribeToTopics() {
    const topics = [
      'motete/osmo/+/status',
      'motete/osmo/+/actions',
      'motete/osmo/+/errors',
      'motete/osmo/+/sensors',
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

      if (topic.includes('/actions')) {
        const unitId = topic.split('/')[2];
        console.log(`🎬 Acción recibida de ${unitId}:`, data);
      }

      if (topic.includes('/errors')) {
        const unitId = topic.split('/')[2];
        console.error(`❌ Error reportado por ${unitId}:`, data);
      }

      if (topic.includes('/sensors')) {
        const unitId = topic.split('/')[2];
        console.log(`🌡️ Datos de sensores de ${unitId}:`, data);
      }
    } catch (error) {
      console.error('❌ Error procesando mensaje:', error);
    }
  }

  sendCommand(unitId, action, params, simulate = false) {
    const command = {
      timestamp: new Date().toISOString(),
      command_id: uuidv4(),
      action: action,
      params: params
    };
    
    if (simulate) {
      // Modo simulación: solo registrar en consola, no publicar al broker
      console.log(`🎭 [SIMULACIÓN] Comando que se habría enviado a ${unitId}:`, command);
      return command.command_id;
    }
    
    // Modo real: verificar conexión saludable y publicar
    if (!this.isConnectionHealthy()) {
      throw new Error('MQTT no conectado o conexión no saludable');
    }
    
    const topic = `motete/director/commands/${unitId}`;
    this.client.publish(topic, JSON.stringify(command), { qos: 1 });
    console.log(`📤 Comando enviado a ${unitId}:`, command);
    return command.command_id;
  }

  getSimulatedOsmos() {
    console.log('🎭 Devolviendo Osmos simulados');
    return [
      {
        unit_id: 'osmo_norte',
        status: 'online',
        battery: 85,
        pumps: { 0: 'active', 1: 'active', 2: 'active', 3: 'active', 4: 'active', 5: 'active', 6: 'active', 7: 'active' },
        lastSeen: new Date()
      },
      {
        unit_id: 'osmo_sur',
        status: 'online', 
        battery: 92,
        pumps: { 0: 'active', 1: 'active', 2: 'active', 3: 'active', 4: 'active', 5: 'active', 6: 'active', 7: 'active' },
        lastSeen: new Date()
      }
    ];
  }

  isConnectionHealthy() {
    // Verificar si la conexión es realmente funcional
    return this.isConnected && this.client && this.client.connected;
  }

  getConnectedOsmos() {
    console.log('getConnectedOsmos', this.isConnected, 'Osmos reales:', this.connectedOsmos.size);
    
    // Solo devolver Osmos reales conectados si la conexión es saludable
    if (!this.isConnectionHealthy()) {
      console.log('⚠️ Conexión MQTT no saludable, no hay Osmos reales');
      return [];
    }
    
    console.log('📡 Devolviendo Osmos reales conectados');
    return Array.from(this.connectedOsmos.values());
  }
}

module.exports = OsmoMQTTClient;
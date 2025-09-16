const mqtt = require('mqtt');
const { v4: uuidv4 } = require('uuid');

class OsmoMQTTClient {
  constructor(password) {
    this.client = null;
    this.connectedOsmos = new Map();
    this.osmoConfigs = new Map(); // ✅ Nuevo: almacenar configuraciones
    this.cooldowns = new Map(); // ✅ unitId -> Map<pumpId, { startedAt, durationMs }>
    this.isConnected = false;
    this.password = password || 'director'; // Fallback por si no se provee
    console.log('🔧 Constructor OsmoMQTTClient iniciado');
    console.log('🔧 Password configurado:', this.password);
  }

  async connect() {
    return new Promise((resolve, reject) => {
      console.log('🔌 Intentando conectar a MQTT con credenciales...');
      console.log('🔌 URL:', 'mqtt://localhost:1883');
      console.log('🔌 Username:', 'director');
      console.log('🔌 Password:', this.password);
      
      this.client = mqtt.connect('mqtt://localhost:1883', {
        // Usar credenciales del director
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
      'motete/osmo/+/response',  // ✅ Agregado para respuestas de comandos
      'motete/osmo/+/command',   // ✅ Agregado para comandos operativos
      'motete/osmo/+/config',    // ✅ Agregado para configuración
      'motete/osmo/discovery'
    ];

    topics.forEach(topic => {
      this.client.subscribe(topic, { qos: 1 });
      console.log(`📡 Suscrito a: ${topic}`);
    });
    
    // Suscribirse también al topic específico del ESP8266
    this.client.subscribe('motete/osmo/osmo_norte/status', { qos: 1 });
    this.client.subscribe('motete/osmo/osmo_norte/response', { qos: 1 });  // ✅ Agregado
    this.client.subscribe('motete/osmo/osmo_norte/command', { qos: 1 });   // ✅ Agregado para comandos
    this.client.subscribe('motete/osmo/osmo_norte/config', { qos: 1 });    // ✅ Agregado para configuración
    console.log(`📡 Suscrito específicamente a: motete/osmo/osmo_norte/status`);
    console.log(`📡 Suscrito específicamente a: motete/osmo/osmo_norte/response`);
    console.log(`📡 Suscrito específicamente a: motete/osmo/osmo_norte/command`);
    console.log(`📡 Suscrito específicamente a: motete/osmo/osmo_norte/config`);
    
    console.log('✅ Todas las suscripciones configuradas');
  }

  handleMessage(topic, message) {
    try {
      console.log(`📩 Mensaje MQTT recibido en topic: ${topic}`);
      console.log(`📩 Contenido del mensaje:`, message.toString());
      
      const data = JSON.parse(message.toString());
      console.log(`📩 Mensaje parseado:`, data);

      if (topic.includes('/status')) {
        const unitId = topic.split('/')[2];
        console.log(`🔍 Procesando status para unitId: ${unitId}`);
        
        this.connectedOsmos.set(unitId, {
          ...data,
          lastSeen: new Date()
        });
        console.log(`💚 Estado actualizado para ${unitId}`);
        console.log(`📊 Total de Osmos conectados: ${this.connectedOsmos.size}`);

        // 🔄 Opcional: sincronizar cooldowns desde status si existe cooldown_remaining
        try {
          if (data && data.pumps) {
            const now = Date.now();
            Object.keys(data.pumps).forEach((pumpKey) => {
              const pump = data.pumps[pumpKey];
              if (pump && typeof pump.cooldown_remaining === 'number') {
                const pumpId = parseInt(pumpKey, 10);
                if (!this.cooldowns.has(unitId)) this.cooldowns.set(unitId, new Map());
                if (pump.cooldown_remaining > 0) {
                  this.cooldowns.get(unitId).set(pumpId, {
                    startedAt: now - Math.max(0, (this._getCooldownDurationMs(unitId, pumpId) - pump.cooldown_remaining)),
                    durationMs: this._getCooldownDurationMs(unitId, pumpId)
                  });
                } else {
                  // cooldown terminado
                  const unitMap = this.cooldowns.get(unitId);
                  if (unitMap) unitMap.delete(pumpId);
                }
              }
            });
          }
        } catch (e) {
          console.warn('⚠️ No se pudo sincronizar cooldowns desde status:', e.message);
        }
      }

      if (topic.includes('/actions')) {
        const unitId = topic.split('/')[2];
        console.log(`🎬 Acción recibida de ${unitId}:`, data);
      }

      if (topic.includes('/errors')) {
        const unitId = topic.split('/')[2];
        console.error(`❌ Error reportado por ${unitId}:`, data);
      }

      if (topic.includes('/response')) {
        const unitId = topic.split('/')[2];
        console.log(`📨 Respuesta de comando recibida de ${unitId}:`, data);
        
        // Actualizar estado del Osmo con la respuesta
        if (this.connectedOsmos.has(unitId)) {
          const osmo = this.connectedOsmos.get(unitId);
          osmo.lastResponse = data;
          osmo.lastResponseTime = new Date();
          this.connectedOsmos.set(unitId, osmo);
          
          // Log de respuesta exitosa o error
          if (data.success) {
            console.log(`✅ Comando exitoso para ${unitId}: ${data.message}`);
          } else {
            console.log(`❌ Comando falló para ${unitId}: ${data.message} (Código: ${data.code})`);
          }
        }
      }

      if (topic.includes('/command')) {
        const unitId = topic.split('/')[2];
        console.log(`🔧 Comando operativo enviado por ${unitId}:`, data);
        console.log(`🔧 ${unitId} envió comando: ${data.action}`);
      }

      if (topic.includes('/config')) {
        const unitId = topic.split('/')[2];
        console.log(`⚙️ Configuración enviada por ${unitId}:`, data);
        console.log(`⚙️ Topic completo: ${topic}`);
        
        // Almacenar configuración de bombas
        if (data.action === 'set_pump_config') {
          console.log(`🔧 ${unitId} configurando bomba ${data.params.pump_id}: activación=${data.params.activation_time}ms, cooldown=${data.params.cooldown_time}ms`);
          
          // Almacenar configuración por bomba
          if (!this.osmoConfigs.has(unitId)) {
            this.osmoConfigs.set(unitId, {});
            console.log(`📝 Creado nuevo Map para ${unitId}`);
          }
          
          const osmoConfig = this.osmoConfigs.get(unitId);
          osmoConfig[`pump_${data.params.pump_id}`] = {
            activationTime: data.params.activation_time,
            cooldownTime: data.params.cooldown_time,
            lastUpdated: new Date()
          };
          
          this.osmoConfigs.set(unitId, osmoConfig);
          console.log(`💾 Configuración almacenada para ${unitId} bomba ${data.params.pump_id}`);
          console.log(`💾 Estado actual del Map para ${unitId}:`, osmoConfig);
        } else {
          console.log(`⚙️ ${unitId} envió configuración: ${data.action}`);
        }
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
    // ✅ Usar la estructura de comando que espera el Arduino
    const command = {
      command_id: `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      action: action,
      params: params,
      timestamp: Date.now()
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

    // ✅ Si es activate_pump y NO estamos en simulación, iniciar cooldown en servidor inmediatamente
    if (!simulate && action === 'activate_pump') {
      const pumpId = params?.pump_id;
      if (typeof pumpId === 'number') {
        // Duración total basada en config: activación + cooldown
        const cfg = this.osmoConfigs.get(unitId)?.[`pump_${pumpId}`];
        const activation = cfg?.activationTime ?? 1000;
        const cooldown = cfg?.cooldownTime ?? 3000;
        const total = activation + cooldown;
        this.startCooldown(unitId, pumpId, total);
      }
    }
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
    console.log('🔍 getConnectedOsmos llamado');
    console.log('📊 Estado de conexión:', this.isConnected);
    console.log('📊 Cliente MQTT:', this.client ? 'existe' : 'no existe');
    console.log('📊 Cliente conectado:', this.client?.connected ? 'sí' : 'no');
    console.log('📊 Total de Osmos en Map:', this.connectedOsmos.size);
    
    // Solo devolver Osmos reales conectados si la conexión es saludable
    if (!this.isConnectionHealthy()) {
      console.log('⚠️ Conexión MQTT no saludable, no hay Osmos reales');
      return [];
    }
    
    console.log('📡 Devolviendo Osmos reales conectados');
    const osmos = Array.from(this.connectedOsmos.values());
    console.log('📊 Osmos a devolver:', osmos);
    return osmos;
  }

  getOsmoConfigs() {
    console.log('⚙️ getOsmoConfigs llamado');
    console.log('📊 Total de configuraciones almacenadas:', this.osmoConfigs.size);
    console.log('📊 Contenido del Map osmoConfigs:', this.osmoConfigs);
    
    // Convertir Map a objeto para facilitar el uso en el frontend
    const configs = {};
    this.osmoConfigs.forEach((config, unitId) => {
      configs[unitId] = config;
      console.log(`📊 Configuración para ${unitId}:`, config);
    });
    
    console.log('📊 Configuraciones a devolver:', configs);
    return configs;
  }

  // ===== Cooldowns (servidor autoritativo) =====
  _getCooldownDurationMs(unitId, pumpId) {
    const cfg = this.osmoConfigs.get(unitId)?.[`pump_${pumpId}`];
    if (cfg && typeof cfg.cooldownTime === 'number') return cfg.cooldownTime;
    // default 3000ms si no hay config
    return 3000;
  }

  startCooldown(unitId, pumpId, durationMs) {
    if (!this.cooldowns.has(unitId)) this.cooldowns.set(unitId, new Map());
    const d = typeof durationMs === 'number' ? durationMs : this._getCooldownDurationMs(unitId, pumpId);
    this.cooldowns.get(unitId).set(pumpId, { startedAt: Date.now(), durationMs: d });
    console.log(`⏱️ [SERVER] Cooldown iniciado: ${unitId} bomba ${pumpId} por ${d}ms`);
    if (typeof this.onCooldownsChanged === 'function') {
      this.onCooldownsChanged();
    }
  }

  getCooldownsSnapshot() {
    const now = Date.now();
    const out = {};
    this.cooldowns.forEach((unitMap, unitId) => {
      const unitOut = {};
      unitMap.forEach((meta, pumpId) => {
        const elapsed = now - meta.startedAt;
        const remaining = Math.max(0, meta.durationMs - elapsed);
        if (remaining > 0) {
          unitOut[pumpId] = { remainingMs: remaining, totalMs: meta.durationMs };
        }
      });
      out[unitId] = unitOut;
    });
    return out;
  }

  getCooldownDurationMs(unitId, pumpId) {
    return this._getCooldownDurationMs(unitId, pumpId);
  }
}

module.exports = OsmoMQTTClient;
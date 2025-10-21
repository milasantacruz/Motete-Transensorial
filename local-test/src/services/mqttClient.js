const mqtt = require('mqtt');
const { v4: uuidv4 } = require('uuid');
const fs = require('fs');
const path = require('path');
const AWS = require('aws-sdk');
require('dotenv').config();

class OsmoMQTTClient {
  constructor() {
    this.client = null;
    this.connectedOsmos = new Map();
    this.osmoConfigs = new Map();
    this.cooldowns = new Map();
    this.isConnected = false;
    this.certificates = null; // Cache para certificados
    
    // Configuración AWS IoT Core desde variables de entorno
    this.awsConfig = {
      endpoint: process.env.AWS_IOT_ENDPOINT,
      clientId: process.env.AWS_IOT_CLIENT_ID || 'director_aws',
      secretName: process.env.AWS_SECRET_NAME || 'aromatorio-mqtt-certificates',
      region: process.env.AWS_REGION || 'us-east-2',
      // Fallback a variables de entorno directas
      caCert: process.env.AWS_CA_CERT || null,
      clientCert: process.env.AWS_CLIENT_CERT || null,
      privateKey: process.env.AWS_PRIVATE_KEY || null,
      // Fallback a archivos
      caCertPath: process.env.AWS_CA_CERT_PATH ? path.join(__dirname, process.env.AWS_CA_CERT_PATH) : null,
      clientCertPath: process.env.AWS_CLIENT_CERT_PATH ? path.join(__dirname, process.env.AWS_CLIENT_CERT_PATH) : null,
      privateKeyPath: process.env.AWS_PRIVATE_KEY_PATH ? path.join(__dirname, process.env.AWS_PRIVATE_KEY_PATH) : null,
    };
    
    // Configurar AWS SDK
    AWS.config.update({ region: this.awsConfig.region });
    this.secretsManager = new AWS.SecretsManager();
    
    console.log('🔧 Constructor OsmoMQTTClient (AWS IoT Core) iniciado');
    console.log('🔧 DEBUG - process.env.AWS_IOT_ENDPOINT:', process.env.AWS_IOT_ENDPOINT);
    console.log('🔧 DEBUG - process.env.AWS_IOT_CLIENT_ID:', process.env.AWS_IOT_CLIENT_ID);
    console.log('🔧 DEBUG - process.env.AWS_SECRET_NAME:', process.env.AWS_SECRET_NAME);
    console.log('🔧 Endpoint:', this.awsConfig.endpoint);
    console.log('🔧 Client ID:', this.awsConfig.clientId);
    console.log('🔧 Secret Name:', this.awsConfig.secretName);
    console.log('🔧 Region:', this.awsConfig.region);
  }

  async getCertificatesFromSecretsManager() {
    try {
      console.log('🔐 Obteniendo certificados desde AWS Secrets Manager...');
      console.log('🔐 Secret Name:', this.awsConfig.secretName);
      
      const result = await this.secretsManager.getSecretValue({
        SecretId: this.awsConfig.secretName
      }).promise();
      
      console.log('✅ Secreto obtenido exitosamente');
      
      // Parsear el JSON del secreto
      const secretData = JSON.parse(result.SecretString);
      
      // Validar que tenemos todos los certificados necesarios
      if (!secretData.AWS_CA_CERT || !secretData.AWS_CLIENT_CERT || !secretData.AWS_PRIVATE_KEY) {
        throw new Error('Faltan certificados en el secreto. Se requieren: AWS_CA_CERT, AWS_CLIENT_CERT, AWS_PRIVATE_KEY');
      }
      
      // Reformatear certificados para que tengan saltos de línea cada 64 caracteres
      this.certificates = {
        ca: this.formatPemCertificate(secretData.AWS_CA_CERT),
        cert: this.formatPemCertificate(secretData.AWS_CLIENT_CERT),
        key: this.formatPemCertificate(secretData.AWS_PRIVATE_KEY)
      };
      
      console.log('✅ Certificados cargados desde Secrets Manager');
      console.log('🔐 CA Cert length:', this.certificates.ca.length);
      console.log('🔐 Client Cert length:', this.certificates.cert.length);
      console.log('🔐 Private Key length:', this.certificates.key.length);
      
      return this.certificates;
    } catch (error) {
      console.error('❌ Error obteniendo certificados desde Secrets Manager:', error.message);
      throw error;
    }
  }

  async getCertificates() {
    // Si ya tenemos certificados en cache, usarlos
    if (this.certificates) {
      console.log('✅ Usando certificados desde cache');
      return this.certificates;
    }
    
    // Intentar obtener desde Secrets Manager primero
    try {
      return await this.getCertificatesFromSecretsManager();
    } catch (secretsError) {
      console.warn('⚠️ No se pudieron obtener certificados desde Secrets Manager:', secretsError.message);
      
      // Fallback a variables de entorno
      if (this.awsConfig.caCert && this.awsConfig.clientCert && this.awsConfig.privateKey) {
        console.log('✅ Usando certificados desde variables de entorno');
        // Reformatear certificados para que tengan saltos de línea cada 64 caracteres
        this.certificates = {
          ca: this.formatPemCertificate(this.awsConfig.caCert),
          cert: this.formatPemCertificate(this.awsConfig.clientCert),
          key: this.formatPemCertificate(this.awsConfig.privateKey)
        };
        return this.certificates;
      }
      
      // Fallback a archivos
      if (this.awsConfig.caCertPath && this.awsConfig.clientCertPath && this.awsConfig.privateKeyPath) {
        console.log('✅ Usando certificados desde archivos');
        
        if (!fs.existsSync(this.awsConfig.caCertPath)) {
          throw new Error(`Certificado CA no encontrado: ${this.awsConfig.caCertPath}`);
        }
        if (!fs.existsSync(this.awsConfig.clientCertPath)) {
          throw new Error(`Certificado de cliente no encontrado: ${this.awsConfig.clientCertPath}`);
        }
        if (!fs.existsSync(this.awsConfig.privateKeyPath)) {
          throw new Error(`Clave privada no encontrada: ${this.awsConfig.privateKeyPath}`);
        }
        
        this.certificates = {
          ca: fs.readFileSync(this.awsConfig.caCertPath),
          cert: fs.readFileSync(this.awsConfig.clientCertPath),
          key: fs.readFileSync(this.awsConfig.privateKeyPath)
        };
        return this.certificates;
      }
      
      throw new Error('No se configuraron certificados (ni en Secrets Manager, ni como variables de entorno ni como archivos)');
    }
  }

  async connect() {
    return new Promise(async (resolve, reject) => {
      try {
        console.log('🔌 Intentando conectar a AWS IoT Core...');
        console.log('🔌 Endpoint:', `mqtts://${this.awsConfig.endpoint}:8883`);
        console.log('🔌 Client ID:', this.awsConfig.clientId);
        
        // Obtener certificados usando el nuevo método
        const certs = await this.getCertificates();
        
        // Conectar a AWS IoT Core con TLS
        this.client = mqtt.connect(`mqtts://${this.awsConfig.endpoint}:8883`, {
          clientId: this.awsConfig.clientId,
          ca: certs.ca,
          cert: certs.cert,
          key: certs.key,
          protocol: 'mqtts',
          port: 8883,
          keepalive: 60,
          reconnectPeriod: 5000,
          connectTimeout: 30000,
          rejectUnauthorized: true,
          clean: true,
        });
        
        console.log('🔌 Cliente MQTT creado, esperando conexión...');

        this.client.on('connect', () => {
          console.log('✅ Director conectado a AWS IoT Core');
          this.isConnected = true;
          this.subscribeToTopics();
          
          // Log periódico para verificar que no hay mensajes
          setInterval(() => {
            console.log(`📊 [Heartbeat] Osmos conectados: ${this.connectedOsmos.size}`);
            if (this.connectedOsmos.size === 0) {
              console.log('⚠️ No hay Osmos conectados. Verifica que ESP82 esté publicando.');
            }
          }, 30000); // Cada 30 segundos
          
          resolve();
        });

        this.client.on('error', (error) => {
          console.error('❌ Error de conexión AWS IoT Core:', error.message);
          this.isConnected = false;
          reject(error);
        });

        this.client.on('close', () => {
          console.log('🔌 Conexión AWS IoT Core cerrada');
          this.isConnected = false;
        });

        this.client.on('offline', () => {
          console.log('📴 Cliente AWS IoT Core offline');
          this.isConnected = false;
        });
        
        this.client.on('reconnect', () => {
          console.log('🔄 Reconectando a AWS IoT Core...');
        });
        
        this.client.on('end', () => {
          console.log('🛑 Cliente MQTT finalizó');
        });
        
        // Evento para errores del stream TLS
        this.client.stream?.on('error', (error) => {
          console.error('❌ Error TLS:', error.message);
          console.error('Código:', error.code);
        });

        this.client.on('message', (topic, message) => {
          this.handleMessage(topic, message);
        });
      } catch (error) {
        console.error('❌ Error en connect():', error.message);
        reject(error);
      }
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
    
    // Prune por freshness (90s = 60s intervalo + 30s margen)
    this._pruneStaleOsmos(90000); // ✅ Cambiado de 10000 a 90000
    
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

  // ===== Housekeeping =====
  _pruneStaleOsmos(maxAgeMs = 90000) { // ✅ 90 segundos (60s de publicación + 30s de margen)
    try {
      const now = Date.now();
      let removed = 0;
      this.connectedOsmos.forEach((osmo, unitId) => {
        const lastSeenTs = osmo?.lastSeen instanceof Date ? osmo.lastSeen.getTime() : Number(new Date(osmo?.lastSeen).getTime());
        const age = now - (Number.isFinite(lastSeenTs) ? lastSeenTs : 0);
        console.log(`⏱️ Freshness check ${unitId} -> lastSeenTs=${lastSeenTs} age=${age}ms (threshold=${maxAgeMs}ms)`);
        if (!Number.isFinite(lastSeenTs) || age > maxAgeMs) {
          this.connectedOsmos.delete(unitId);
          removed += 1;
          console.log(`🗑️ Pruned ${unitId} por inactividad (${age}ms)`);
        }
      });
      if (removed > 0) {
        console.log(`🧹 Prune completo. Eliminados: ${removed}. Restantes: ${this.connectedOsmos.size}`);
      }
    } catch (e) {
      console.warn('⚠️ Error en _pruneStaleOsmos:', e.message);
    }
  }

  /**
   * Reformatea un certificado PEM para que tenga saltos de línea cada 64 caracteres
   * @param {string} cert - Certificado en formato PEM (puede estar en una sola línea)
   * @returns {string} - Certificado con formato PEM correcto
   */
  formatPemCertificate(cert) {
    if (!cert) return cert;
    
    // Si ya tiene saltos de línea, devolverlo tal como está
    if (cert.includes('\n')) {
      return cert;
    }
    
    // Extraer el tipo de certificado (BEGIN/END lines)
    const beginMatch = cert.match(/-----BEGIN [^-]+-----/);
    const endMatch = cert.match(/-----END [^-]+-----/);
    
    if (!beginMatch || !endMatch) {
      console.warn('⚠️ Certificado no tiene formato PEM válido, devolviendo tal como está');
      return cert;
    }
    
    const beginLine = beginMatch[0];
    const endLine = endMatch[0];
    
    // Extraer el contenido del certificado (sin las líneas BEGIN/END)
    const content = cert.substring(beginLine.length, cert.lastIndexOf(endLine)).trim();
    
    // Dividir el contenido en líneas de 64 caracteres
    const lines = [];
    for (let i = 0; i < content.length; i += 64) {
      lines.push(content.substring(i, i + 64));
    }
    
    // Reconstruir el certificado con formato correcto
    return `${beginLine}\n${lines.join('\n')}\n${endLine}`;
  }
}

module.exports = OsmoMQTTClient;
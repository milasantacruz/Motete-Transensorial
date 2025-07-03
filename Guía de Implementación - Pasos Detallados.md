# Guía de Implementación - Broker MQTT Motete Transensorial
## 📋 Pasos Detallados para Desarrolladores

> **Objetivo**: Implementar el sistema completo de comunicación MQTT para controlar 3 unidades Osmo v5 desde una Raspberry Pi.

---

## 🎯 **FASE 1: Preparación del Entorno**

### Paso 1.1: Configurar Raspberry Pi
**⏱️ Tiempo estimado: 60 minutos**

1. **Instalar Raspberry Pi OS**
   ```bash
   # Descargar Raspberry Pi Imager
   # Flashear SD card con Raspberry Pi OS Lite (64-bit)
   # Habilitar SSH antes de flashear
   ```

2. **Conexión inicial por SSH**
   ```bash
   ssh pi@[IP_DE_LA_RASPBERRY]
   # Contraseña por defecto: raspberry
   ```

3. **Actualizar sistema**
   ```bash
   sudo apt update
   sudo apt upgrade -y
   sudo reboot
   ```

4. **Configurar WiFi (opcional)**
   ```bash
   sudo raspi-config
   # Ir a: Network Options > Wi-Fi
   # Configurar SSID y contraseña
   ```

### Paso 1.2: Instalar Dependencias Base
**⏱️ Tiempo estimado: 60 minutos**

1. **Instalar Node.js**
   ```bash
   curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
   sudo apt-get install -y nodejs
   node --version  # Verificar instalación
   npm --version   # Verificar npm
   ```

2. **Instalar herramientas adicionales**
   ```bash
   sudo apt install -y git vim mosquitto mosquitto-clients
   ```

3. **Verificar instalaciones**
   ```bash
   mosquitto --help
   git --version
   ```

---

## 🦟 **FASE 2: Configuración del Broker Mosquitto**

### Paso 2.1: Configurar Mosquitto
**⏱️ 
 estimado: 60 minutos**

1. **Detener servicio por defecto**
   ```bash
   sudo systemctl stop mosquitto
   sudo systemctl disable mosquitto
   ```

2. **Crear directorio de configuración**
   ```bash
   mkdir -p ~/motete/config
   cd ~/motete/config
   ```

3. **Crear archivo de configuración**
   ```bash
   nano mosquitto.conf
   ```
   
   **Contenido del archivo:**
   ```conf
   # Puerto MQTT
   port 1883
   
   # Deshabilitar conexiones anónimas
   allow_anonymous false
   
   # Archivo de contraseñas
   password_file /home/pi/motete/config/passwd
   
   # Logs
   log_dest file /home/pi/motete/logs/mosquitto.log
   log_type all
   
   # Persistencia
   persistence true
   persistence_location /home/pi/motete/data/
   
   # Configuración de QoS
   max_queued_messages 1000
   message_size_limit 8192
   
   # Keep alive
   keepalive_interval 60
   ```

4. **Crear directorios necesarios**
   ```bash
   mkdir -p ~/motete/logs
   mkdir -p ~/motete/data
   ```

### Paso 2.2: Configurar Usuarios MQTT
**⏱️ Tiempo estimado: 40 minutos**

1. **Crear archivo de contraseñas**
   ```bash
   cd ~/motete/config
   
   # Crear usuario director
   mosquitto_passwd -c passwd director
   # Introducir contraseña cuando se solicite
   
   # Agregar usuarios para cada Osmo
   mosquitto_passwd passwd osmo_norte
   mosquitto_passwd passwd osmo_sur
   mosquitto_passwd passwd osmo_este
   ```

2. **Verificar archivo de usuarios**
   ```bash
   cat passwd
   # Debe mostrar 4 líneas con usuarios y contraseñas encriptadas
   ```

### Paso 2.3: Probar Mosquitto
**⏱️ Tiempo estimado: 30 minutos**

1. **Iniciar broker manualmente**
   ```bash
   cd ~/motete/config
   mosquitto -c mosquitto.conf
   ```

2. **En otra terminal, probar conexión**
   ```bash
   # Terminal 2: Suscribirse a un topic
   mosquitto_sub -h localhost -t "test/topic" -u director -P [CONTRASEÑA_DIRECTOR]
   
   # Terminal 3: Publicar mensaje
   mosquitto_pub -h localhost -t "test/topic" -m "Hola Motete" -u director -P [CONTRASEÑA_DIRECTOR]
   ```

3. **Si funciona, detener broker**
   ```bash
   # Ctrl+C en la terminal del broker
   ```

---

## 📦 **FASE 3: Desarrollo de la Aplicación Node.js**

### Paso 3.1: Inicializar Proyecto Node.js
**⏱️ Tiempo estimado: 60 minutos**

1. **Crear estructura del proyecto**
   ```bash
   cd ~/motete
   mkdir src
   cd src
   npm init -y
   ```

2. **Instalar dependencias**
   ```bash
   npm install mqtt express ws yaml uuid moment
   npm install --save-dev nodemon
   ```

3. **Crear estructura de carpetas**
   ```bash
   mkdir controllers services utils routes public
   mkdir partituras logs
   ```

### Paso 3.2: Crear Cliente MQTT Base
**⏱️ Tiempo estimado: 20 minutos**

1. **Crear archivo principal**
   ```bash
   nano services/mqttClient.js
   ```

   **Contenido del archivo:**
   ```javascript
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
           password: 'TU_CONTRASEÑA_DIRECTOR_AQUI', // ⚠️ CAMBIAR
           clientId: 'director_' + Math.random().toString(16).substr(2, 8),
           keepalive: 60,
           reconnectPeriod: 5000
         });
   
         this.client.on('connect', () => {
           console.log('✅ Director conectado al broker MQTT');
           this.isConnected = true;
           this.subscribeToTopics();
           resolve();
         });
   
         this.client.on('error', (error) => {
           console.error('❌ Error MQTT:', error);
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
         'motete/osmo/+/actions', 
         'motete/osmo/+/errors',
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
   
         // Aquí procesaremos los diferentes tipos de mensajes
         if (topic.includes('/status')) {
           this.handleStatusMessage(topic, data);
         } else if (topic.includes('/actions')) {
           this.handleActionMessage(topic, data);
         } else if (topic.includes('/discovery')) {
           this.handleDiscoveryMessage(data);
         }
       } catch (error) {
         console.error('❌ Error parsing mensaje:', error);
       }
     }
   
     handleStatusMessage(topic, data) {
       // Extraer ID de la unidad del topic
       const unitId = topic.split('/')[2];
       this.connectedOsmos.set(unitId, {
         ...data,
         lastSeen: new Date()
       });
       console.log(`💚 Estado actualizado para ${unitId}`);
     }
   
     handleActionMessage(topic, data) {
       console.log(`✅ Acción confirmada:`, data);
     }
   
     handleDiscoveryMessage(data) {
       console.log(`🔍 Nuevo Osmo detectado:`, data);
     }
   
     sendCommand(unitId, action, params) {
       if (!this.isConnected) {
         throw new Error('MQTT no conectado');
       }
   
       const command = {
         timestamp: new Date().toISOString(),
         command_id: uuidv4(),
         action: action,
         params: params
       };
   
       const topic = `motete/director/commands/${unitId}`;
       
       this.client.publish(topic, JSON.stringify(command), { qos: 1 }, (error) => {
         if (error) {
           console.error(`❌ Error enviando comando a ${unitId}:`, error);
         } else {
           console.log(`📤 Comando enviado a ${unitId}:`, command);
         }
       });
   
       return command.command_id;
     }
   
     getConnectedOsmos() {
       return Array.from(this.connectedOsmos.entries()).map(([id, data]) => ({
         id,
         ...data
       }));
     }
   }
   
   module.exports = OsmoMQTTClient;
   ```

### Paso 3.3: Crear Servidor Express
**⏱️ Tiempo estimado: 60 minutos**

1. **Crear servidor principal**
   ```bash
   nano app.js
   ```

   **Contenido del archivo:**
   ```javascript
   const express = require('express');
   const path = require('path');
   const OsmoMQTTClient = require('./services/mqttClient');
   
   const app = express();
   const PORT = 3000;
   
   // Middleware
   app.use(express.json());
   app.use(express.static('public'));
   
   // Inicializar cliente MQTT
   const mqttClient = new OsmoMQTTClient();
   
   // Rutas básicas
   app.get('/', (req, res) => {
     res.sendFile(path.join(__dirname, 'public', 'index.html'));
   });
   
   app.get('/api/status', (req, res) => {
     res.json({
       mqtt_connected: mqttClient.isConnected,
       connected_osmos: mqttClient.getConnectedOsmos(),
       timestamp: new Date().toISOString()
     });
   });
   
   app.post('/api/command/:unitId', (req, res) => {
     try {
       const { unitId } = req.params;
       const { action, params } = req.body;
       
       const commandId = mqttClient.sendCommand(unitId, action, params);
       
       res.json({
         success: true,
         command_id: commandId,
         message: `Comando ${action} enviado a ${unitId}`
       });
     } catch (error) {
       res.status(500).json({
         success: false,
         error: error.message
       });
     }
   });
   
   // Iniciar servidor
   async function startServer() {
     try {
       console.log('🚀 Iniciando Motete Director...');
       
       // Conectar MQTT
       await mqttClient.connect();
       
       // Iniciar servidor web
       app.listen(PORT, () => {
         console.log(`🌐 Servidor web en http://localhost:${PORT}`);
         console.log('✅ Sistema listo para recibir Osmos');
       });
     } catch (error) {
       console.error('❌ Error iniciando servidor:', error);
       process.exit(1);
     }
   }
   
   startServer();
   ```

2. **Actualizar package.json**
   ```bash
   nano package.json
   ```
   
   **Agregar scripts:**
   ```json
   {
     "scripts": {
       "start": "node app.js",
       "dev": "nodemon app.js"
     }
   }
   ```

### Paso 3.4: Crear Interfaz Web Básica
**⏱️ Tiempo estimado: 60 minutos**

1. **Crear página principal**
   ```bash
   nano public/index.html
   ```

   **Contenido del archivo:**
   ```html
   <!DOCTYPE html>
   <html lang="es">
   <head>
       <meta charset="UTF-8">
       <meta name="viewport" content="width=device-width, initial-scale=1.0">
       <title>Motete Transensorial - Control</title>
       <style>
           body {
               font-family: Arial, sans-serif;
               margin: 20px;
               background-color: #f5f5f5;
           }
           .container {
               max-width: 1200px;
               margin: 0 auto;
           }
           .osmo-card {
               background: white;
               border-radius: 10px;
               padding: 20px;
               margin: 10px;
               box-shadow: 0 2px 5px rgba(0,0,0,0.1);
           }
           .status-online { border-left: 5px solid green; }
           .status-offline { border-left: 5px solid red; }
           button {
               background: #007bff;
               color: white;
               border: none;
               padding: 10px 20px;
               border-radius: 5px;
               cursor: pointer;
               margin: 5px;
           }
           button:hover { background: #0056b3; }
           .pump-grid {
               display: grid;
               grid-template-columns: repeat(4, 1fr);
               gap: 10px;
               margin: 10px 0;
           }
           .pump-button {
               padding: 15px;
               font-size: 12px;
           }
       </style>
   </head>
   <body>
       <div class="container">
           <h1>🎼 Motete Transensorial - Control</h1>
           
           <div id="status">
               <h2>Estado del Sistema</h2>
               <p id="mqtt-status">Conectando...</p>
           </div>
   
           <div id="osmos-container">
               <!-- Los Osmos se cargarán aquí dinámicamente -->
           </div>
   
           <script>
               let osmos = [];
   
               async function loadStatus() {
                   try {
                       const response = await fetch('/api/status');
                       const data = await response.json();
                       
                       document.getElementById('mqtt-status').innerHTML = 
                           data.mqtt_connected ? '✅ MQTT Conectado' : '❌ MQTT Desconectado';
                       
                       osmos = data.connected_osmos;
                       renderOsmos();
                   } catch (error) {
                       console.error('Error loading status:', error);
                   }
               }
   
               function renderOsmos() {
                   const container = document.getElementById('osmos-container');
                   
                   if (osmos.length === 0) {
                       container.innerHTML = '<p>⏳ Esperando conexión de Osmos...</p>';
                       return;
                   }
   
                   container.innerHTML = osmos.map(osmo => `
                       <div class="osmo-card status-online">
                           <h3>🌸 ${osmo.id}</h3>
                           <p><strong>Estado:</strong> ${osmo.status}</p>
                           <p><strong>Temperatura:</strong> ${osmo.temperature}°C</p>
                           <p><strong>WiFi:</strong> ${osmo.wifi_strength} dBm</p>
                           
                           <h4>Bombas Aromáticas</h4>
                           <div class="pump-grid">
                               ${Array.from({length: 8}, (_, i) => i + 1).map(pumpNum => `
                                   <button class="pump-button" onclick="activatePump('${osmo.id}', ${pumpNum})">
                                       Bomba ${pumpNum}
                                       <br>
                                       <small>Nivel: ${osmo.pumps?.[pumpNum]?.level || 0}%</small>
                                   </button>
                               `).join('')}
                           </div>
                           
                           <button onclick="testBlower('${osmo.id}')">🌬️ Test Soplador</button>
                       </div>
                   `).join('');
               }
   
               async function activatePump(unitId, pumpNumber) {
                   try {
                       const response = await fetch(`/api/command/${unitId}`, {
                           method: 'POST',
                           headers: { 'Content-Type': 'application/json' },
                           body: JSON.stringify({
                               action: 'aroma',
                               params: {
                                   pump: pumpNumber,
                                   duration: 3000,
                                   intensity: 80
                               }
                           })
                       });
   
                       const result = await response.json();
                       console.log('Comando enviado:', result);
                       alert(`Bomba ${pumpNumber} activada en ${unitId}`);
                   } catch (error) {
                       console.error('Error:', error);
                       alert('Error enviando comando');
                   }
               }
   
               async function testBlower(unitId) {
                   try {
                       const response = await fetch(`/api/command/${unitId}`, {
                           method: 'POST',
                           headers: { 'Content-Type': 'application/json' },
                           body: JSON.stringify({
                               action: 'blower',
                               params: { duration: 2000, speed: 100 }
                           })
                       });
                       
                       alert(`Soplador activado en ${unitId}`);
                   } catch (error) {
                       console.error('Error:', error);
                   }
               }
   
               // Actualizar estado cada 5 segundos
               setInterval(loadStatus, 5000);
               loadStatus();
           </script>
       </div>
   </body>
   </html>
   ```

---

## 🧪 **FASE 4: Pruebas y Validación**

### Paso 4.1: Probar Sistema Sin Osmos
**⏱️ Tiempo estimado: 30 minutos**

1. **Iniciar broker Mosquitto**
   ```bash
   cd ~/motete/config
   mosquitto -c mosquitto.conf &
   ```

2. **Iniciar aplicación Node.js**
   ```bash
   cd ~/motete/src
   npm start
   ```

3. **Verificar en navegador**
   - Abrir `http://[IP_RASPBERRY]:3000`
   - Verificar que muestre "MQTT Conectado"
   - Debe mostrar "Esperando conexión de Osmos"

### Paso 4.2: Simular Osmo con Mosquitto Client
**⏱️ Tiempo estimado: 40 minutos**

1. **En nueva terminal, simular discovery**
   ```bash
   mosquitto_pub -h localhost -u osmo_norte -P [CONTRASEÑA_OSMO_NORTE] \
     -t "motete/osmo/discovery" \
     -m '{"unit_id":"osmo_norte","type":"osmo_v5","firmware":"1.0.0"}'
   ```

2. **Simular estado de Osmo**
   ```bash
   mosquitto_pub -h localhost -u osmo_norte -P [CONTRASEÑA_OSMO_NORTE] \
     -t "motete/osmo/osmo_norte/status" \
     -m '{
       "timestamp":"2025-01-14T10:30:01.000Z",
       "unit_id":"osmo_norte",
       "status":"ready",
       "pumps":{
         "1":{"active":false,"level":95},
         "2":{"active":false,"level":87},
         "3":{"active":false,"level":72},
         "4":{"active":false,"level":45},
         "5":{"active":false,"level":88},
         "6":{"active":false,"level":91},
         "7":{"active":false,"level":23},
         "8":{"active":false,"level":67}
       },
       "temperature":245,
       "wifi_strength":-45
     }'
   ```

3. **Verificar en interfaz web**
   - Recargar página
   - Debe aparecer "osmo_norte" como conectado
   - Probar botones de bombas

### Paso 4.3: Monitorear Comandos
**⏱️ Tiempo estimado: 15 minutos**

1. **Escuchar comandos desde la web**
   ```bash
   mosquitto_sub -h localhost -u osmo_norte -P [CONTRASEÑA_OSMO_NORTE] \
     -t "motete/director/commands/osmo_norte"
   ```

2. **Probar desde interfaz web**
   - Hacer clic en "Bomba 1"
   - Verificar que llegue comando JSON

---

## 🔧 **FASE 5: Scripts de Automatización**

### Paso 5.1: Script de Inicio Automático
**⏱️ Tiempo estimado: 30 minutos**

1. **Crear script de inicio**
   ```bash
   nano ~/motete/start.sh
   ```

   **Contenido:**
   ```bash
   #!/bin/bash
   
   echo "🚀 Iniciando Motete Transensorial..."
   
   # Matar procesos existentes
   pkill -f mosquitto
   pkill -f node
   
   # Esperar un momento
   sleep 2
   
   # Iniciar broker Mosquitto
   cd /home/pi/motete/config
   mosquitto -c mosquitto.conf &
   echo "✅ Broker MQTT iniciado"
   
   # Esperar que mosquitto esté listo
   sleep 3
   
   # Iniciar aplicación Node.js
   cd /home/pi/motete/src
   npm start &
   echo "✅ Aplicación iniciada"
   
   echo "🎼 Sistema listo en http://localhost:3000"
   ```

2. **Hacer ejecutable**
   ```bash
   chmod +x ~/motete/start.sh
   ```

3. **Probar script**
   ```bash
   ~/motete/start.sh
   ```

### Paso 5.2: Script de Parada
**⏱️ Tiempo estimado: 15 minutos**

1. **Crear script de parada**
   ```bash
   nano ~/motete/stop.sh
   ```

   **Contenido:**
   ```bash
   #!/bin/bash
   
   echo "🛑 Deteniendo Motete Transensorial..."
   
   pkill -f mosquitto
   pkill -f node
   
   echo "✅ Sistema detenido"
   ```

2. **Hacer ejecutable**
   ```bash
   chmod +x ~/motete/stop.sh
   ```

---

## ✅ **FASE 6: Verificación Final**

### Checklist de Verificación

- [ ] Raspberry Pi configurada y actualizada
- [ ] Node.js instalado y funcionando
- [ ] Mosquitto instalado y configurado
- [ ] Usuarios MQTT creados correctamente
- [ ] Aplicación Node.js inicia sin errores
- [ ] Interfaz web accesible desde navegador
- [ ] Estado MQTT muestra "Conectado"
- [ ] Comandos de prueba se envían correctamente
- [ ] Scripts de inicio/parada funcionan
- [ ] Sistema completo funcional

### Comandos de Diagnóstico

```bash
# Verificar procesos
ps aux | grep mosquitto
ps aux | grep node

# Verificar puertos
netstat -tlnp | grep :1883  # MQTT
netstat -tlnp | grep :3000  # Web

# Ver logs
tail -f ~/motete/logs/mosquitto.log

# Test conexión MQTT
mosquitto_pub -h localhost -u director -P [CONTRASEÑA] -t "test" -m "hello"
```

---

## 📚 **Recursos Adicionales**

### Solución de Problemas Comunes

1. **Error "MQTT no conectado"**
   - Verificar que Mosquitto esté corriendo
   - Verificar contraseñas en mqttClient.js

2. **"Cannot connect to broker"**
   - Verificar puerto 1883 abierto
   - Verificar archivo de configuración

3. **Interfaz web no carga**
   - Verificar puerto 3000 disponible
   - Verificar que Node.js esté corriendo

### Próximos Pasos de Desarrollo

1. **Implementar parser YAML** para partituras
2. **Crear firmware ESP32** para Osmos reales
3. **Agregar WebSockets** para updates en tiempo real
4. **Implementar sistema de logging** robusto
5. **Crear tests automatizados**

---

## 🎯 **Resultado Esperado**

Al completar esta guía tendrás:
- ✅ Broker MQTT funcionando en Raspberry Pi
- ✅ Aplicación web para controlar Osmos
- ✅ Sistema base para conectar 3 unidades Osmo v5
- ✅ Interfaz para probar comandos individuales
- ✅ Base sólida para implementar partituras complejas

**¡El núcleo del "Motete Transensorial" estará listo para recibir los Osmos v5!** 🎼🌸 
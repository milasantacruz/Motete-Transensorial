# Guía de Implementación Local del Broker MQTT (Nivel 0)

> **Objetivo**: Implementar un entorno de desarrollo local completo para el "Motete Transensorial", ejecutando el broker Mosquitto y la aplicación de control Node.js en tu propia máquina.

---

## 🎯 **FASE 1: Preparación del Entorno Local**

### Paso 1.1: Instalar Dependencias de Software

**1. Instalar Node.js:**
- Ve a la página oficial de [Node.js](https://nodejs.org/) y descarga la versión LTS.
- Ejecuta el instalador y sigue los pasos, asegurándote de que tanto `node` como `npm` se agreguen al PATH de tu sistema.
- Abre una nueva terminal (PowerShell o CMD) y verifica la instalación:
  ```bash
  node --version
  npm --version
  ```

**2. Instalar Mosquitto en Windows:**
- Ve a la [página de descargas de Mosquitto](https://mosquitto.org/download/).
- Descarga el instalador para tu versión de Windows (ej. `mosquitto-2.0.18-install-windows-x64.exe`).
- Ejecuta el instalador. Es importante que durante la instalación, te asegures de que las herramientas `mosquitto.exe`, `mosquitto_pub.exe`, `mosquitto_sub.exe` y `mosquitto_passwd.exe` se agreguen a tu PATH. Generalmente, el instalador lo hace por ti si seleccionas la opción correspondiente.
- Reinicia tu terminal para que reconozca los nuevos comandos y verifica la instalación:
  ```bash
  mosquitto --help
  ```

### Paso 1.2: Crear la Estructura del Proyecto

1.  Abre una terminal y crea una carpeta para el proyecto.
    ```bash
    mkdir motete-transensorial
    cd motete-transensorial
    ```
2.  Crea la estructura de carpetas que usaremos.
    ```bash
    mkdir config logs data src
    ```

---

## 🦟 **FASE 2: Configuración del Broker Mosquitto**

### Paso 2.1: Crear Archivo de Configuración

1.  Navega al directorio de configuración que creaste.
    ```bash
    cd config
    ```
2.  Crea un archivo de configuración llamado `mosquitto.conf`. Puedes usar cualquier editor de texto.
    ```bash
    # En PowerShell puedes usar:
    New-Item mosquitto.conf
    # Luego abre el archivo con `notepad mosquitto.conf` o VSCode `code .`
    ```
3.  **Añade el siguiente contenido al archivo `mosquitto.conf`:**

    > **Nota importante:** Las rutas de los archivos deben usar `slashes` (`/`) incluso en Windows, ya que así lo requiere Mosquitto. Reemplaza `C:/ruta/a/tu/proyecto` con la ruta absoluta donde creaste la carpeta `motete-transensorial`.

    ```conf
    # Puerto MQTT
    port 1883
    
    # Deshabilitar conexiones anónimas
    allow_anonymous false
    
    # Archivo de contraseñas (usa la ruta absoluta a tu proyecto)
    password_file C:/ruta/a/tu/proyecto/motete-transensorial/config/passwd
    
    # Logs (usa la ruta absoluta a tu proyecto)
    log_dest file C:/ruta/a/tu/proyecto/motete-transensorial/logs/mosquitto.log
    log_type all
    
    # Persistencia (usa la ruta absoluta a tu proyecto)
    persistence true
    persistence_location C:/ruta/a/tu/proyecto/motete-transensorial/data/
    
    # Keep alive
    keepalive_interval 60
    ```
    **Ejemplo de ruta en Windows:** `C:/Users/TuUsuario/Documents/motete-transensorial/config/passwd`

### Paso 2.2: Configurar Usuarios MQTT

1.  Asegúrate de estar en el directorio `config`.
2.  Ejecuta los siguientes comandos para crear los usuarios. Se te pedirá que introduzcas una contraseña para cada uno. **Anota estas contraseñas, las necesitarás más tarde.**

    ```bash
    # Crear el archivo de contraseñas y el primer usuario 'director'
    mosquitto_passwd -c passwd director
    
    # Agregar los usuarios para los Osmos
    mosquitto_passwd passwd osmo_norte
    mosquitto_passwd passwd osmo_sur
    mosquitto_passwd passwd osmo_este
    ```
3.  Verifica que el archivo `passwd` se haya creado y contenga 4 líneas.

---

## 📦 **FASE 3: Desarrollo de la Aplicación Node.js**

### Paso 3.1: Inicializar Proyecto y Dependencias

1.  Navega al directorio `src` de tu proyecto.
    ```bash
    cd ../src  # Si estabas en 'config'
    ```
2.  Inicializa un proyecto de Node.js.
    ```bash
    npm init -y
    ```
3.  Instala las dependencias necesarias.
    ```bash
    npm install mqtt express uuid
    npm install --save-dev nodemon
    ```
4.  Crea la estructura de carpetas dentro de `src`.
    ```bash
    mkdir services public
    ```

### Paso 3.2: Crear Cliente MQTT (`mqttClient.js`)

1.  Crea el archivo `src/services/mqttClient.js`.
2.  **Pega el siguiente código:**

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
            // ⚠️ CAMBIA ESTO por la contraseña que creaste para el usuario 'director'
            password: 'TU_CONTRASEÑA_DIRECTOR_AQUI', 
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
    ```

### Paso 3.3: Crear Servidor Express (`app.js`)

1.  En la raíz de `src`, crea el archivo `app.js`.
2.  **Pega el siguiente código:**

    ```javascript
    const express = require('express');
    const path = require('path');
    const OsmoMQTTClient = require('./services/mqttClient');
    
    const app = express();
    const PORT = 3000;
    
    app.use(express.json());
    app.use(express.static(path.join(__dirname, 'public')));
    
    const mqttClient = new OsmoMQTTClient();
    
    app.get('/api/status', (req, res) => {
      res.json({
        mqtt_connected: mqttClient.isConnected,
        connected_osmos: mqttClient.getConnectedOsmos(),
      });
    });
    
    app.post('/api/command/:unitId', (req, res) => {
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
        console.error('❌ No se pudo iniciar el servidor:', error);
        process.exit(1);
      }
    }
    
    startServer();
    ```

### Paso 3.4: Crear Interfaz Web (`index.html`)

1.  Crea el archivo `src/public/index.html`.
2.  **Pega el siguiente código HTML y JavaScript:**

    ```html
    <!DOCTYPE html>
    <html lang="es">
    <head>
        <meta charset="UTF-8">
        <title>Control Motete Transensorial (Local)</title>
        <style>
            body { font-family: sans-serif; margin: 2em; }
            .osmo-card { border: 1px solid #ccc; border-left: 5px solid green; padding: 1em; margin-bottom: 1em; }
            button { margin-top: 10px; }
        </style>
    </head>
    <body>
        <h1>Control Motete Transensorial (Local)</h1>
        <p>Estado MQTT: <b id="mqtt-status">...</b></p>
        <div id="osmos-container"></div>
    
        <script>
            async function loadStatus() {
                const response = await fetch('/api/status');
                const data = await response.json();
                document.getElementById('mqtt-status').innerText = data.mqtt_connected ? '✅ Conectado' : '❌ Desconectado';
                
                const container = document.getElementById('osmos-container');
                if (data.connected_osmos.length === 0) {
                    container.innerHTML = '<p>Esperando Osmos...</p>';
                    return;
                }
    
                container.innerHTML = data.connected_osmos.map(osmo => `
                    <div class="osmo-card">
                        <h3>${osmo.unit_id}</h3>
                        <p>Estado: ${osmo.status}</p>
                        <p>Batería: ${osmo.battery}%</p>
                        <button onclick="sendCommand('${osmo.unit_id}', 'aroma', { pump: 1, duration: 1000 })">Activar Bomba 1</button>
                    </div>
                `).join('');
            }
    
            async function sendCommand(unitId, action, params) {
                await fetch(`/api/command/${unitId}`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action, params })
                });
                alert(`Comando '${action}' enviado a ${unitId}`);
            }
    
            setInterval(loadStatus, 2000);
            loadStatus();
        </script>
    </body>
    </html>
    ```

---

## 🧪 **FASE 4: Pruebas y Validación**

### Paso 4.1: Iniciar el Sistema

1.  **Abre una primera terminal** y navega a la carpeta `config` de tu proyecto. Inicia el broker Mosquitto.
    ```bash
    # Desde la raíz del proyecto:
    cd config
    mosquitto -c mosquitto.conf -v
    # La opción -v (verbose) te mostrará los logs en tiempo real.
    ```
    Si ves un error sobre `password_file`, asegúrate de que la ruta en `mosquitto.conf` es **absoluta y correcta**.

2.  **Abre una segunda terminal** y navega a la carpeta `src`. Inicia la aplicación Node.js.
    ```bash
    # Desde la raíz del proyecto:
    cd src
    node app.js
    ```
    Deberías ver los mensajes "Director conectado al broker MQTT" y "Servidor web escuchando...".

3.  **Abre tu navegador** y ve a `http://localhost:3000`. Deberías ver la interfaz web con el estado "MQTT: ✅ Conectado" y "Esperando Osmos...".

### Paso 4.2: Simular un Osmo

1.  **Abre una tercera terminal**. Vamos a simular que `osmo_norte` se conecta y envía su estado.
2.  Usa `mosquitto_pub` para enviar un mensaje de estado. **Recuerda usar la contraseña que creaste para `osmo_norte`**.

    ```bash
    # Reemplaza [CONTRASEÑA] con la contraseña correcta
   mosquitto_pub -h localhost -t "motete/osmo/osmo_norte/status" -u "osmo_norte" -P "[CONTRASEÑA]" -f ./simulation/osmo_norte_status.json
    ```
3.  **Refresca la página web en tu navegador.** Ahora deberías ver la tarjeta de control para `osmo_norte`.

### Paso 4.3: Probar un Comando

1.  En la interfaz web, haz clic en el botón **"Activar Bomba 1"**.
2.  Observa la terminal donde corre la aplicación Node.js (`app.js`). Deberías ver un mensaje "Comando enviado a osmo_norte".
3.  Si tuvieras un dispositivo real (o un script de simulación) escuchando los comandos, recibiría la instrucción.

---

## ✅ **Resultado Esperado**

Al completar esta guía, tendrás un entorno de desarrollo local completamente funcional que te permitirá:
- Ejecutar un broker MQTT seguro con usuarios.
- Correr una aplicación Node.js que se conecta al broker.
- Controlar dispositivos simulados a través de una interfaz web simple.
- Probar y depurar la lógica de comunicación MQTT antes de pasar a hardware físico. 
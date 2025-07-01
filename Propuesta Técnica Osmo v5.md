Osmo v5 (12.6.25)

### Propuesta Técnica: Osmo v5 - Plataforma de Creación Multisensorial

El objetivo es evolucionar Osmo v5 de un dispositivo individual a una red de instrumentos multimedia, permitiendo la composición y ejecución de obras complejas que sincronicen aromas, audio y video.

---

### 1. Arquitectura General: "Director y Orquesta"

El sistema se divide en un cerebro central (Director) y múltiples unidades de ejecución (Instrumentos).

- El Director (Cerebro Central) 🧠  
      
    

- Hardware: Una Raspberry Pi (modelo 4 o superior), operando en modo "headless" (sin monitor/teclado) y conectada por cable Ethernet al router para máxima estabilidad.
    
- Software Principal (Backend): Node.js.
    
- Responsabilidades:
    

- Alojar el servidor web con la interfaz de control.
    
- Correr el broker MQTT para gestionar la comunicación con todas las unidades Osmo.
    
- Interpretar las partituras en formato YAML y enviar los comandos en el momento preciso.
    
- Manejar la reproducción local de audio y video.
    
- Gestionar las conexiones por WebSockets para la interactividad en tiempo real.
    

- Los Instrumentos (Unidades Osmo v5) 👃  
      
    

- Hardware: Un microcontrolador ESP32 en cada unidad.
    
- Responsabilidades:
    

- Conectarse a la red y suscribirse a sus órdenes en el broker MQTT.
    
- Ejecutar acciones físicas con precisión (activar bombas, blowers, gestionar sensores).
    
- Reportar su estado a El Director.
    

---

### 2. Comunicación y Red

La fiabilidad y la respuesta en tiempo real son la máxima prioridad.

- Red Física: Se creará una red Wi-Fi dedicada y aislada usando un router de buena calidad. Solo los dispositivos de la obra (Raspberry Pi, ESP32, tablets de control) se conectarán a ella.
    
- Protocolo de Comandos (Director -> Instrumentos): MQTT. Se usará una Calidad de Servicio (QoS 1) para garantizar que todas las órdenes de activación de aromas lleguen a su destino.
    
- Protocolo de Interactividad (Controles -> Director): WebSockets. Permite una comunicación bidireccional instantánea entre los dispositivos de control (tablets/celulares) y el servidor Node.js, ideal para el modo "Jam Session" multiusuario.
    

---

### 3. Composición: El Lenguaje de Partituras

Las obras se escribirán en un formato simple, legible y potente.

- Formato de Archivo: YAML (.yml).  
      
    
- Estructura: Cada partitura contendrá metadatos (título, autor) y una línea de tiempo. La línea de tiempo estará compuesta por puntos de tiempo que pueden contener una lista de múltiples eventos simultáneos.  
      
    
- Ejemplo de "Acorde" Multisensorial:  
      
    
- YAML
    

- time: "01:30.500"

  events:

    - { target: "unidad_norte", action: "aroma", params: { pump: 2 } }

    - { target: "unidad_sur", action: "aroma", params: { pump: 6 } }

    - { target: "master", action: "video", params: { file: "escena_mar.mp4" } }

-   
    
-   
    

---

### 4. Interfaz y Experiencia de Usuario

La plataforma será controlada a través de una interfaz web moderna y accesible desde cualquier navegador.

- Editor Visual de Partituras: Una herramienta de arrastrar y soltar (drag-and-drop) en una línea de tiempo que permitirá a los usuarios componer visualmente, generando automáticamente el archivo YAML de la obra.
    
- Banco de Obras: Una biblioteca dentro de la interfaz para cargar, gestionar y ejecutar las partituras guardadas en la Raspberry Pi.
    
- Modo "Jam Session": Un panel de control en vivo que permitirá a uno o varios usuarios (desde distintas tablets, por ejemplo) disparar aromas, sonidos y efectos en tiempo real de forma colaborativa.
    

  

---

  

Python vs [Node.js](http://node.js) para Backend

  

|   |   |   |   |
|---|---|---|---|
|Característica|Python (con FastAPI/Flask)|Node.js (con Express)|Comentario|
|Solidez|Extremadamente sólido. Usado en backends masivos (Instagram).|Extremadamente sólido. Usado en backends masivos (Netflix).|Empate. Ambos son de nivel productivo.|
|Escalabilidad (Conexiones)|Buena, especialmente con frameworks async.|Superior. Su arquitectura está optimizada para I/O y conexiones.|Ventaja para Node.js.|
|Gestión de WebSockets|Buena.|Excelente. El ecosistema y el lenguaje están diseñados para esto.|Ventaja clara para Node.js.|
|Lógica General y Scripts|Excelente. Muy legible y directo para la lógica de la partitura.|Muy bueno.|Ligera ventaja para Python en simplicidad.|
|Ecosistema Web|Muy bueno.|Excelente. Es el lenguaje nativo de la web.|Ventaja para Node.js.|
# 🏗️ Patrones de Diseño en C++ y Arduino
## Guía Completa con Ejemplos Prácticos

---

## 📊 TABLA COMPLETA DE COMPONENTES Y PATRONES

### **Análisis Detallado de tu Arquitectura**

| Componente | Archivo | Patrón de Diseño | ¿Por qué? |
|------------|---------|------------------|------------|---------|---------|
| **MainController** | `main_controller.h/cpp` | **Composite** | Coordina múltiples componentes especializados 
| **NetworkManager** | `network_manager.h/cpp` | **Adapter** | Adapta interfaz MQTT a tu sistema 
| **PumpController** | `pump_controller.h/cpp` | **Command** | Encapsula operaciones de bombas 
| **StatusPublisher** | `status_publisher.h/cpp` | **Observer** | Publica cambios de estado del sistema 
| **Config** | `config.h/cpp` | **Singleton** | Configuración global única del sistema
| **Arduino Sketch** | `plantilla_modular.ino` | **Template Method** | Define esqueleto del programa principal 


---

## 📚 ¿Qué son los Patrones de Diseño?

Los **patrones de diseño** son **soluciones probadas y reutilizables** para problemas comunes en programación. Son como **plantillas de construcción** que puedes usar una y otra vez.

### **¿Por qué usar patrones?**
- ✅ **Reutilización** de código probado
- ✅ **Mantenibilidad** del código
- ✅ **Legibilidad** y comprensión
- ✅ **Escalabilidad** del proyecto
- ✅ **Estándares** de la industria

---

## 🎯 Patrón 1: Singleton (Instancia Única)

### **¿Qué hace?**
Garantiza que una clase tenga **solo UNA instancia** y proporciona acceso global a ella.

### **¿Cuándo usarlo?**
- Configuración global del sistema
- Conexión WiFi única
- Gestor de memoria
- Logger del sistema

### **Ejemplo Práctico:**
```cpp
class ConfiguracionSistema {
private:
    // Constructor privado - nadie puede crear instancias
    ConfiguracionSistema() {
        ssid = "MiWiFi";
        password = "12345678";
        mqttServer = "192.168.1.100";
    }
    
    // La única instancia que existe
    static ConfiguracionSistema* instancia;
    
    // Variables de configuración
    string ssid;
    string password;
    string mqttServer;
    
public:
    // Método para obtener la única instancia
    static ConfiguracionSistema* obtenerInstancia() {
        if (instancia == nullptr) {
            instancia = new ConfiguracionSistema();
        }
        return instancia;
    }
    
    // Getters para acceder a la configuración
    string getSSID() { return ssid; }
    string getPassword() { return password; }
    string getMQTTServer() { return mqttServer; }
    
    // Prevenir copias
    ConfiguracionSistema(const ConfiguracionSistema&) = delete;
    ConfiguracionSistema& operator=(const ConfiguracionSistema&) = delete;
};

// Inicializar la variable estática
ConfiguracionSistema* ConfiguracionSistema::instancia = nullptr;

// Uso:
void setup() {
    ConfiguracionSistema* config = ConfiguracionSistema::obtenerInstancia();
    WiFi.begin(config->getSSID().c_str(), config->getPassword().c_str());
}
```

---

## 🏭 Patrón 2: Factory (Fábrica)

### **¿Qué hace?**
Crea objetos sin especificar su clase exacta. La fábrica decide qué tipo crear basándose en parámetros.

### **¿Cuándo usarlo?**
- Crear diferentes tipos de sensores
- Crear diferentes tipos de bombas
- Crear diferentes tipos de comunicación
- Crear diferentes tipos de actuadores

### **Ejemplo Práctico:**
```cpp
// Clase base abstracta
class Sensor {
public:
    virtual void inicializar() = 0;
    virtual float leer() = 0;
    virtual string getTipo() = 0;
    virtual ~Sensor() {}
};

// Sensores específicos
class SensorTemperatura : public Sensor {
private:
    int pin;
    
public:
    SensorTemperatura(int p) : pin(p) {}
    
    void inicializar() override {
        pinMode(pin, INPUT);
    }
    
    float leer() override {
        return analogRead(pin) * 0.488; // Convertir a °C
    }
    
    string getTipo() override {
        return "Temperatura";
    }
};

class SensorHumedad : public Sensor {
private:
    int pin;
    
public:
    SensorHumedad(int p) : pin(p) {}
    
    void inicializar() override {
        pinMode(pin, INPUT);
    }
    
    float leer() override {
        return analogRead(pin) * 0.097; // Convertir a %HR
    }
    
    string getTipo() override {
        return "Humedad";
    }
};

// La Fábrica
class FabricaSensores {
public:
    static Sensor* crearSensor(string tipo, int pin) {
        if (tipo == "temperatura") {
            return new SensorTemperatura(pin);
        }
        else if (tipo == "humedad") {
            return new SensorHumedad(pin);
        }
        return nullptr;
    }
};

// Uso:
void setup() {
    Sensor* sensorTemp = FabricaSensores::crearSensor("temperatura", A0);
    Sensor* sensorHum = FabricaSensores::crearSensor("humedad", A1);
    
    sensorTemp->inicializar();
    sensorHum->inicializar();
    
    float temp = sensorTemp->leer();
    float hum = sensorHum->leer();
}
```

---

## 👁️ Patrón 3: Observer (Observador)

### **¿Qué hace?**
Define una dependencia uno-a-muchos entre objetos. Cuando un objeto cambia, todos sus dependientes son notificados automáticamente.

### **¿Cuándo usarlo?**
- Sistema de notificaciones
- Monitoreo de sensores
- Sistema de alertas
- Actualización de displays

### **Ejemplo Práctico:**
```cpp
// Interfaz del observador
class Observador {
public:
    virtual void actualizar(string mensaje) = 0;
    virtual ~Observador() {}
};

// Observadores específicos
class DisplayLCD : public Observador {
private:
    string nombre;
    
public:
    DisplayLCD(string n) : nombre(n) {}
    
    void actualizar(string mensaje) override {
        Serial.print(nombre + " LCD: ");
        Serial.println(mensaje);
        // Aquí iría el código para actualizar el LCD
    }
};

class Buzzer : public Observador {
private:
    int pin;
    
public:
    Buzzer(int p) : pin(p) {
        pinMode(pin, OUTPUT);
    }
    
    void actualizar(string mensaje) override {
        if (mensaje.find("ALERTA") != string::npos) {
            // Activar buzzer si hay alerta
            digitalWrite(pin, HIGH);
            delay(100);
            digitalWrite(pin, LOW);
        }
    }
};

// El sujeto que notifica
class SistemaMonitoreo {
private:
    vector<Observador*> observadores;
    float temperatura;
    float humedad;
    
public:
    void agregarObservador(Observador* obs) {
        observadores.push_back(obs);
    }
    
    void quitarObservador(Observador* obs) {
        // Buscar y remover el observador
        auto it = find(observadores.begin(), observadores.end(), obs);
        if (it != observadores.end()) {
            observadores.erase(it);
        }
    }
    
    void notificarTodos(string mensaje) {
        for (auto obs : observadores) {
            obs->actualizar(mensaje);
        }
    }
    
    void actualizarMediciones(float temp, float hum) {
        temperatura = temp;
        humedad = hum;
        
        // Notificar cambios
        if (temperatura > 30) {
            notificarTodos("ALERTA: Temperatura alta: " + to_string(temperatura) + "°C");
        }
        
        if (humedad < 20) {
            notificarTodos("ALERTA: Humedad baja: " + to_string(humedad) + "%");
        }
    }
};

// Uso:
void setup() {
    SistemaMonitoreo sistema;
    DisplayLCD lcd("Principal");
    Buzzer buzzer(8);
    
    sistema.agregarObservador(&lcd);
    sistema.agregarObservador(&buzzer);
    
    // Simular mediciones
    sistema.actualizarMediciones(35.5, 15.0);
}
```

---

## 🎯 Patrón 4: Strategy (Estrategia)

### **¿Qué hace?**
Define una familia de algoritmos, encapsula cada uno y los hace intercambiables. Permite cambiar el algoritmo en tiempo de ejecución.

### **¿Cuándo usarlo?**
- Diferentes modos de operación
- Diferentes algoritmos de control
- Diferentes métodos de comunicación
- Diferentes estrategias de riego

### **Ejemplo Práctico:**
```cpp
// Interfaz de estrategia
class EstrategiaRiego {
public:
    virtual void regar() = 0;
    virtual string getNombre() = 0;
    virtual ~EstrategiaRiego() {}
};

// Estrategias específicas
class RiegoManual : public EstrategiaRiego {
public:
    void regar() override {
        Serial.println("🌧️ Riego MANUAL activado");
        // Activar bombas por tiempo fijo
        activarBomba(1, 5000); // 5 segundos
        activarBomba(2, 5000);
    }
    
    string getNombre() override {
        return "Manual";
    }
    
private:
    void activarBomba(int bomba, int tiempo) {
        Serial.print("Bomba ");
        Serial.print(bomba);
        Serial.print(" activada por ");
        Serial.print(tiempo);
        Serial.println(" ms");
    }
};

class RiegoAutomatico : public EstrategiaRiego {
private:
    float umbralHumedad;
    
public:
    RiegoAutomatico(float umbral) : umbralHumedad(umbral) {}
    
    void regar() override {
        Serial.println("🤖 Riego AUTOMÁTICO activado");
        float humedad = leerHumedad();
        
        if (humedad < umbralHumedad) {
            Serial.println("Humedad baja, activando riego...");
            activarBomba(1, 10000); // 10 segundos
        } else {
            Serial.println("Humedad adecuada, no es necesario regar");
        }
    }
    
    string getNombre() override {
        return "Automático";
    }
    
private:
    float leerHumedad() {
        return random(15, 85); // Simulación
    }
    
    void activarBomba(int bomba, int tiempo) {
        Serial.print("Bomba ");
        Serial.print(bomba);
        Serial.print(" activada por ");
        Serial.print(tiempo);
        Serial.println(" ms");
    }
};

class RiegoProgramado : public EstrategiaRiego {
private:
    int horaInicio;
    int duracion;
    
public:
    RiegoProgramado(int hora, int dur) : horaInicio(hora), duracion(dur) {}
    
    void regar() override {
        Serial.println("⏰ Riego PROGRAMADO activado");
        Serial.print("Inicio: ");
        Serial.print(horaInicio);
        Serial.print(":00, Duración: ");
        Serial.print(duracion);
        Serial.println(" minutos");
        
        // Activar bombas según programación
        for (int i = 0; i < duracion; i++) {
            activarBomba(1, 60000); // 1 minuto
            activarBomba(2, 60000);
            delay(60000); // Esperar 1 minuto
        }
    }
    
    string getNombre() override {
        return "Programado";
    }
    
private:
    void activarBomba(int bomba, int tiempo) {
        Serial.print("Bomba ");
        Serial.print(bomba);
        Serial.print(" activada por ");
        Serial.print(tiempo);
        Serial.println(" ms");
    }
};

// Contexto que usa las estrategias
class ControladorRiego {
private:
    EstrategiaRiego* estrategia;
    
public:
    void setEstrategia(EstrategiaRiego* est) {
        estrategia = est;
    }
    
    void ejecutarRiego() {
        if (estrategia) {
            Serial.print("Estrategia actual: ");
            Serial.println(estrategia->getNombre());
            estrategia->regar();
        }
    }
};

// Uso:
void setup() {
    ControladorRiego controlador;
    
    // Cambiar estrategias
    RiegoManual manual;
    RiegoAutomatico automatico(30.0);
    RiegoProgramado programado(6, 15); // 6:00 AM, 15 minutos
    
    controlador.setEstrategia(&manual);
    controlador.ejecutarRiego();
    
    controlador.setEstrategia(&automatico);
    controlador.ejecutarRiego();
    
    controlador.setEstrategia(&programado);
    controlador.ejecutarRiego();
}
```

---

## 📋 Patrón 5: Command (Comando)

### **¿Qué hace?**
Encapsula una solicitud como un objeto, permitiendo parametrizar clientes con diferentes solicitudes, colas o logs de solicitudes.

### **¿Cuándo usarlo?**
- Comandos MQTT
- Cola de operaciones
- Sistema de macros
- Operaciones deshacer/rehacer

### **Ejemplo Práctico:**
```cpp
// Interfaz del comando
class Comando {
public:
    virtual void ejecutar() = 0;
    virtual void deshacer() = 0;
    virtual string getDescripcion() = 0;
    virtual ~Comando() {}
};

// Comandos específicos
class ComandoActivarBomba : public Comando {
private:
    int idBomba;
    bool estadoAnterior;
    
public:
    ComandoActivarBomba(int id) : idBomba(id), estadoAnterior(false) {}
    
    void ejecutar() override {
        estadoAnterior = obtenerEstadoBomba(idBomba);
        activarBomba(idBomba, true);
        Serial.print("✅ Bomba ");
        Serial.print(idBomba);
        Serial.println(" ACTIVADA");
    }
    
    void deshacer() override {
        activarBomba(idBomba, estadoAnterior);
        Serial.print("↩️ Bomba ");
        Serial.print(idBomba);
        Serial.print(" restaurada a: ");
        Serial.println(estadoAnterior ? "ACTIVA" : "INACTIVA");
    }
    
    string getDescripcion() override {
        return "Activar Bomba " + to_string(idBomba);
    }
    
private:
    bool obtenerEstadoBomba(int id) {
        // Simulación - en realidad leería el estado real
        return random(0, 2) == 1;
    }
    
    void activarBomba(int id, bool estado) {
        Serial.print("Bomba ");
        Serial.print(id);
        Serial.print(" -> ");
        Serial.println(estado ? "ON" : "OFF");
    }
};

class ComandoCambiarNivel : public Comando {
private:
    int idBomba;
    int nivelNuevo;
    int nivelAnterior;
    
public:
    ComandoCambiarNivel(int id, int nivel) : idBomba(id), nivelNuevo(nivel), nivelAnterior(0) {}
    
    void ejecutar() override {
        nivelAnterior = obtenerNivelBomba(idBomba);
        cambiarNivelBomba(idBomba, nivelNuevo);
        Serial.print("📊 Bomba ");
        Serial.print(idBomba);
        Serial.print(" nivel cambiado a: ");
        Serial.println(nivelNuevo);
    }
    
    void deshacer() override {
        cambiarNivelBomba(idBomba, nivelAnterior);
        Serial.print("↩️ Bomba ");
        Serial.print(idBomba);
        Serial.print(" nivel restaurado a: ");
        Serial.println(nivelAnterior);
    }
    
    string getDescripcion() override {
        return "Cambiar nivel bomba " + to_string(idBomba) + " a " + to_string(nivelNuevo);
    }
    
private:
    int obtenerNivelBomba(int id) {
        return random(0, 101); // Simulación
    }
    
    void cambiarNivelBomba(int id, int nivel) {
        Serial.print("Bomba ");
        Serial.print(id);
        Serial.print(" nivel -> ");
        Serial.println(nivel);
    }
};

// Invocador que ejecuta comandos
class ControladorComandos {
private:
    vector<Comando*> historial;
    
public:
    void ejecutarComando(Comando* comando) {
        comando->ejecutar();
        historial.push_back(comando);
        Serial.print("💾 Comando guardado: ");
        Serial.println(comando->getDescripcion());
    }
    
    void deshacerUltimo() {
        if (!historial.empty()) {
            Comando* ultimo = historial.back();
            ultimo->deshacer();
            historial.pop_back();
            Serial.println("↩️ Último comando deshecho");
        } else {
            Serial.println("❌ No hay comandos para deshacer");
        }
    }
    
    void mostrarHistorial() {
        Serial.println("📋 Historial de comandos:");
        for (int i = 0; i < historial.size(); i++) {
            Serial.print(i + 1);
            Serial.print(". ");
            Serial.println(historial[i]->getDescripcion());
        }
    }
};

// Uso:
void setup() {
    ControladorComandos controlador;
    
    // Crear y ejecutar comandos
    Comando* cmd1 = new ComandoActivarBomba(1);
    Comando* cmd2 = new ComandoCambiarNivel(1, 75);
    Comando* cmd3 = new ComandoActivarBomba(2);
    
    controlador.ejecutarComando(cmd1);
    controlador.ejecutarComando(cmd2);
    controlador.ejecutarComando(cmd3);
    
    controlador.mostrarHistorial();
    
    // Deshacer último comando
    controlador.deshacerUltimo();
}
```

---

## 🔌 Patrón 6: Adapter (Adaptador)

### **¿Qué hace?**
Permite que interfaces incompatibles trabajen juntas. Convierte la interfaz de una clase en otra interfaz que el cliente espera.

### **¿Cuándo usarlo?**
- Conectar sensores viejos con nuevos sistemas
- Adaptar librerías de terceros
- Conectar diferentes protocolos de comunicación
- Integrar hardware legacy

### **Ejemplo Práctico:**
```cpp
// Interfaz antigua (sistema legacy)
class SensorViejo {
public:
    int obtenerValor() {
        return analogRead(A0);
    }
    
    bool estaActivo() {
        return digitalRead(13) == HIGH;
    }
};

// Nueva interfaz estándar
class SensorNuevo {
public:
    virtual float leerValor() = 0;
    virtual bool estaConectado() = 0;
    virtual string getUnidad() = 0;
    virtual ~SensorNuevo() {}
};

// El adaptador
class AdaptadorSensor : public SensorNuevo {
private:
    SensorViejo* sensorViejo;
    float factorConversion;
    string unidad;
    
public:
    AdaptadorSensor(SensorViejo* sensor, float factor, string unit) 
        : sensorViejo(sensor), factorConversion(factor), unidad(unit) {}
    
    float leerValor() override {
        // Convierte el valor del sensor viejo al nuevo formato
        return sensorViejo->obtenerValor() * factorConversion;
    }
    
    bool estaConectado() override {
        // Convierte el estado del sensor viejo
        return sensorViejo->estaActivo();
    }
    
    string getUnidad() override {
        return unidad;
    }
};

// Uso:
void setup() {
    SensorViejo sensorViejo;
    
    // Crear adaptadores para diferentes tipos de medición
    AdaptadorSensor sensorTemp(&sensorViejo, 0.488, "°C");
    AdaptadorSensor sensorHum(&sensorViejo, 0.097, "%HR");
    AdaptadorSensor sensorLuz(&sensorViejo, 0.0049, "lux");
    
    // Usar como sensores nuevos
    Serial.print("Temperatura: ");
    Serial.print(sensorTemp.leerValor());
    Serial.println(sensorTemp.getUnidad());
    
    Serial.print("Humedad: ");
    Serial.print(sensorHum.leerValor());
    Serial.println(sensorHum.getUnidad());
    
    Serial.print("Luz: ");
    Serial.print(sensorLuz.leerValor());
    Serial.println(sensorLuz.getUnidad());
}
```

---

## �� Patrón 7: Template Method (Método Plantilla)

### **¿Qué hace?**
Define el esqueleto de un algoritmo en una clase base, dejando que las subclases implementen pasos específicos.

### **¿Cuándo usarlo?**
- Algoritmos con pasos similares pero diferentes implementaciones
- Procesos de medición
- Secuencias de inicialización
- Protocolos de comunicación

### **Ejemplo Práctico:**
```cpp
// Clase base con el método plantilla
class ProcesoMedicion {
public:
    // Este es el método plantilla
    void ejecutarProceso() {
        Serial.println("🚀 Iniciando proceso de medición...");
        
        inicializar();
        realizarMedicion();
        procesarDatos();
        guardarResultados();
        generarReporte();
        
        Serial.println("✅ Proceso completado");
    }
    
protected:
    // Métodos que las subclases DEBEN implementar
    virtual void inicializar() = 0;
    virtual void realizarMedicion() = 0;
    virtual void procesarDatos() = 0;
    virtual void guardarResultados() = 0;
    
    // Método opcional que puede ser sobrescrito
    virtual void generarReporte() {
        Serial.println("📊 Generando reporte estándar...");
    }
    
    // Método común que todas las subclases pueden usar
    void esperarEstabilizacion() {
        Serial.println("⏳ Esperando estabilización del sensor...");
        delay(2000);
    }
};

// Implementación específica para temperatura
class MedicionTemperatura : public ProcesoMedicion {
private:
    int pin;
    float temperatura;
    
protected:
    void inicializar() override {
        Serial.println("🌡️ Inicializando sensor de temperatura");
        pin = A0;
        pinMode(pin, INPUT);
        esperarEstabilizacion();
    }
    
    void realizarMedicion() override {
        Serial.println("📏 Realizando medición de temperatura");
        int lectura = analogRead(pin);
        temperatura = lectura * 0.488; // Convertir a °C
        Serial.print("Lectura: ");
        Serial.print(lectura);
        Serial.print(" -> ");
        Serial.print(temperatura);
        Serial.println("°C");
    }
    
    void procesarDatos() override {
        Serial.println("🧮 Procesando datos de temperatura");
        if (temperatura > 30) {
            Serial.println("⚠️ Temperatura alta detectada!");
        }
    }
    
    void guardarResultados() override {
        Serial.println("💾 Guardando temperatura en memoria");
        // Aquí iría el código para guardar en EEPROM o SD
    }
    
    void generarReporte() override {
        Serial.println("📊 Generando reporte de temperatura");
        Serial.print("Temperatura final: ");
        Serial.print(temperatura);
        Serial.println("°C");
    }
};

// Implementación específica para humedad
class MedicionHumedad : public ProcesoMedicion {
private:
    int pin;
    float humedad;
    
protected:
    void inicializar() override {
        Serial.println("💧 Inicializando sensor de humedad");
        pin = A1;
        pinMode(pin, INPUT);
        esperarEstabilizacion();
    }
    
    void realizarMedicion() override {
        Serial.println("📏 Realizando medición de humedad");
        int lectura = analogRead(pin);
        humedad = lectura * 0.097; // Convertir a %HR
        Serial.print("Lectura: ");
        Serial.print(lectura);
        Serial.print(" -> ");
        Serial.print(humedad);
        Serial.println("%HR");
    }
    
    void procesarDatos() override {
        Serial.println("🧮 Procesando datos de humedad");
        if (humedad < 20) {
            Serial.println("⚠️ Humedad baja detectada!");
        }
    }
    
    void guardarResultados() override {
        Serial.println("💾 Guardando humedad en memoria");
        // Aquí iría el código para guardar en EEPROM o SD
    }
};

// Uso:
void setup() {
    Serial.begin(115200);
    
    // Crear diferentes tipos de medición
    MedicionTemperatura medicionTemp;
    MedicionHumedad medicionHum;
    
    // Ejecutar procesos (cada uno con su implementación específica)
    medicionTemp.ejecutarProceso();
    Serial.println();
    medicionHum.ejecutarProceso();
}
```

---

## 🎯 ¿Cuándo usar cada patrón?

| Patrón | Problema que resuelve | Ejemplo en Arduino |
|--------|----------------------|-------------------|
| **Singleton** | Necesitas UNA instancia global | Configuración, WiFi, Logger |
| **Factory** | No sabes qué tipo crear | Diferentes sensores/actuadores |
| **Observer** | Muchos objetos necesitan notificaciones | Sistema de alertas, displays |
| **Strategy** | Quieres cambiar comportamiento | Modos de operación, algoritmos |
| **Command** | Quieres hacer operaciones en cola | Comandos MQTT, macros |
| **Adapter** | Tienes interfaces incompatibles | Sensores legacy, librerías |
| **Template Method** | Algoritmos similares con pasos diferentes | Procesos de medición |

---



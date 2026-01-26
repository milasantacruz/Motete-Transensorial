# 🔧 Troubleshooting: Serial Monitor no muestra nada

> **Nota:** Este documento asume que el hardware funciona correctamente (el sketch `plantilla_AWS_IOT` funciona), por lo que se enfoca en problemas específicos del código de `plantilla_server_embeded`.

## 🔍 Problema Identificado: Uso de deviceConfig en Constructor

### ⚠️ PROBLEMA PRINCIPAL (CAUSA DEL BLOQUEO)

**El constructor de `PumpController` se ejecuta ANTES de `setup()` y usa `deviceConfig`:**

```cpp
// En pump_controller.cpp línea 4-14
PumpController::PumpController() : pumpCount(0) {
    // ...
    pumps[i].activationTime = deviceConfig.pumpDefaults.activationTime;  // ← PROBLEMA
    pumps[i].cooldownTime = deviceConfig.pumpDefaults.cooldownTime;      // ← PROBLEMA
}
```

**Orden de ejecución (INCORRECTO):**
1. Se crea `MainController controller` (global, se ejecuta ANTES de `setup()`)
2. Esto crea `PumpController pumpController` como miembro
3. Se ejecuta el constructor de `PumpController` → **usa `deviceConfig`**
4. Si hay algún problema con `deviceConfig`, el código se cuelga
5. **NUNCA llega a `setup()`** → **NUNCA se ejecuta `Serial.begin()`** → **No ves ningún print**

**Solución aplicada:**
- Mover el uso de `deviceConfig` del constructor al método `initialize()`
- El constructor ahora solo inicializa valores temporales
- `deviceConfig` se usa después de que `Serial.begin()` se ejecute

### 1. ⚠️ Falta de Delay Inicial

**Problema:** El código no tenía un delay después de `Serial.begin()`, por lo que los primeros mensajes se perdían si el Serial Monitor no estaba abierto a tiempo.

**Solución aplicada:** Se agregó `delay(2000)` después de `Serial.begin(115200)`.

**Comparación con `plantilla_AWS_IOT`:**
- `plantilla_AWS_IOT` tiene `delay(8000)` después de `Serial.begin()`
- `plantilla_server_embeded` ahora también tiene delay

### 4. ⚠️ Posible Problema con WiFi.begin()

**Problema:** Si el WiFi no se conecta, el código puede quedarse bloqueado o no mostrar mensajes de error claros.

**Verificación:** Revisa en `network_manager.cpp` línea 34:
- El loop de conexión WiFi tiene un límite de 20 intentos
- Si falla, debería mostrar "Error: No se pudo conectar a WiFi"
- Pero si hay un problema antes de llegar ahí, no verás nada

### 5. ⚠️ Configuración de WiFi Incorrecta

**Problema:** Si las credenciales WiFi en `config.cpp` son incorrectas, el código puede fallar silenciosamente.

**Verificación:**
1. Revisa `config.cpp` líneas 4-7
2. Verifica que el SSID y password sean correctos
3. Asegúrate de que la red WiFi esté disponible

### 6. ⚠️ Problema de Inicialización de Objetos

**Problema:** Los objetos `NetworkManager` y `PumpController` se crean como miembros de la clase antes de que `Serial.begin()` se ejecute.

**Verificación:** El orden de inicialización es:
1. Se crea `MainController controller` (global)
2. Se ejecuta `setup()` → `controller.initialize()`
3. Dentro de `initialize()` se llama a `Serial.begin()`

Si hay un problema en el constructor de `NetworkManager` o `PumpController` que use Serial antes de `Serial.begin()`, no verás nada.

## ✅ Soluciones Recomendadas (en orden de prioridad)

### Solución 1: Agregar Delay Inicial (RECOMENDADO)

Modifica `main_controller.cpp`:

```cpp
void MainController::initialize() {
    Serial.begin(115200);
    delay(2000);  // ← AGREGAR ESTA LÍNEA (dar tiempo para abrir Serial Monitor)
    Serial.println();
    Serial.println("=== Motete Transensorial - Piano Server ===");
    // ... resto del código sin cambios
}
```

### Solución 2: Abrir Serial Monitor Antes de Subir

1. Abre el Serial Monitor (Ctrl+Shift+M)
2. Configura velocidad a **115200**
3. **Luego** sube el código
4. Presiona RESET después de subir

### Solución 3: Agregar Logs de Debug en el Constructor

Si el problema persiste, agrega logs en el constructor para ver dónde se cuelga:

```cpp
// En main_controller.cpp
MainController::MainController() : lastStatusPublish(0) {
    // NO usar Serial aquí, aún no está inicializado
}
```

### Solución 4: Verificar Configuración WiFi

Revisa `config.cpp` y asegúrate de que:
- El SSID sea correcto (sin espacios extra)
- El password sea correcto
- La red WiFi esté disponible y funcionando

## 🔍 Mensajes que Deberías Ver

Si todo funciona correctamente, deberías ver en el Serial Monitor:

```
=== Motete Transensorial - Piano Server ===
Dispositivo: OSMO_PIANO_001
Bombas: 4
Puerto servidor: 80
PumpController inicializado con 4 bombas
Bomba 0 en pin 12
Bomba 1 en pin 13
Bomba 2 en pin 14
Bomba 3 en pin 15
Conectando a WiFi...
.....
WiFi conectado!
IP address: 192.168.1.XXX
Servidor web iniciado
Accede a: http://192.168.1.XXX/piano.html
Sistema inicializado correctamente
```

## 🆘 Debugging Avanzado

### Si aún no ves nada después de agregar el delay:

1. **Verifica que el código se compiló correctamente:**
   - No debe haber errores de compilación
   - Debe mostrar "Sketch uses XXXXX bytes"

2. **Compara con `plantilla_AWS_IOT`:**
   - Ambos usan la misma estructura básica
   - La diferencia principal es el delay inicial
   - Revisa si hay otras diferencias en la inicialización

3. **Agrega un test mínimo:**
   - Crea un sketch nuevo con solo `Serial.begin(115200)` y `Serial.println("TEST")`
   - Si esto funciona, el problema está en la inicialización de objetos

## 📝 Checklist Rápido

- [ ] Agregué `delay(2000)` después de `Serial.begin(115200)`
- [ ] Abrí el Serial Monitor ANTES de subir el código
- [ ] Configuré la velocidad a 115200 en Serial Monitor
- [ ] Presioné RESET después de subir
- [ ] Verifiqué que las credenciales WiFi sean correctas
- [ ] El sketch `plantilla_AWS_IOT` funciona (hardware OK)

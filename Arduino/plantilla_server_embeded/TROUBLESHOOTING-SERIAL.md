# 🔧 Troubleshooting: Serial Monitor no muestra nada

> **Nota:** Este documento asume que el hardware funciona correctamente (el sketch `plantilla_AWS_IOT` funciona), por lo que se enfoca en problemas específicos del código de `plantilla_server_embeded`.

## 🔍 Problema Identificado y Solución Aplicada

### ⚠️ PROBLEMA PRINCIPAL: Objetos como miembros de clase vs punteros

**El problema era que `plantilla_server_embeded` usaba objetos como miembros de clase:**

```cpp
// ANTES (problemático):
class MainController {
private:
    NetworkManager networkManager;   // ← Objeto, constructor se ejecuta ANTES de setup()
    PumpController pumpController;   // ← Objeto, constructor se ejecuta ANTES de setup()
};
```

**Esto causaba que:**
1. Al crear `MainController controller` (variable global), se ejecutaban los constructores de `NetworkManager` y `PumpController` ANTES de `setup()`
2. Estos constructores podían usar librerías o variables que aún no estaban inicializadas
3. El código se colgaba ANTES de llegar a `setup()` → ANTES de `Serial.begin()` → No ves ningún print

### ✅ SOLUCIÓN APLICADA: Usar punteros (igual que plantilla_AWS_IOT)

```cpp
// AHORA (correcto):
class MainController {
private:
    NetworkManager* networkManager;   // ← Puntero, se crea en initialize() DESPUÉS de Serial.begin()
    PumpController* pumpController;   // ← Puntero, se crea en initialize() DESPUÉS de Serial.begin()
};
```

**Orden de ejecución (CORRECTO):**
1. Se crea `MainController controller` (global)
2. Constructor de `MainController` solo inicializa punteros a `nullptr` (rápido, sin problemas)
3. Se ejecuta `setup()` → `controller.initialize()`
4. Dentro de `initialize()`:
   - Se ejecuta `Serial.begin(115200)`
   - Se ejecuta `delay(2000)`
   - Se crean los objetos con `new PumpController()` y `new NetworkManager()`
   - Se inicializan los objetos
5. **Ahora SÍ ves los prints porque Serial ya está inicializado**

### 📋 Comparación con plantilla_AWS_IOT

| Característica | plantilla_AWS_IOT | plantilla_server_embeded (antes) | plantilla_server_embeded (ahora) |
|----------------|-------------------|----------------------------------|----------------------------------|
| PumpController | `PumpController*` (puntero) | `PumpController` (objeto) | `PumpController*` (puntero) ✅ |
| NetworkManager | `NetworkManager` (objeto) | `NetworkManager` (objeto) | `NetworkManager*` (puntero) ✅ |
| Creación de objetos | `new` en constructor | automática (constructor de clase) | `new` en initialize() ✅ |
| Serial funciona | ✅ Sí | ❌ No | ✅ Sí |

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

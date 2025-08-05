# Integración del **Ticker-Tone** con la Interfaz de Control

## 1. Objetivo
Integrar el secuenciador visual **Ticker-Tone** dentro de la aplicación web de monitoreo y control de Osmos para:
1. Permitir la edición y reproducción de secuencias de aromas.
2. Detectar automáticamente los Osmos conectados y representar una fila por cada uno (8 bombas).
3. Enviar comandos MQTT de forma transparente desde la interfaz.
4. Disponer de un modo **Solo simulación** para pruebas sin hardware.

## 2. Panorama general de la arquitectura
```
Usuario ──► Navegador (Ticker-Tone) ──► Express REST API ──► mqtt.js ──► Broker MQTT ──► Osmos
                                         ▲                                             │
                                         │◄────────────────  /api/status (polling) ◄────┘
```
* **Polling**: El navegador consulta `/api/status` cada 2 s para descubrir/actualizar Osmos.
* **Comandos**: El navegador envía `POST /api/command/:unitId` con JSON `{ action, params }`.
* **Simulación**: Si `simulate=true` en la query string, el backend **no** publica al broker.

## 3. Requerimientos funcionales
| ID | Descripción |
|----|-------------|
| RF-01 | Añadir un enlace de navegación a `/tickertone` desde la página principal. |
| RF-02 | Mostrar una fila por Osmo detectado con 8 celdas (bombas) y un botón *Enable/Disable*. |
| RF-03 | Proveer controles globales: duración del disparo (ms), BPM, y checkbox **Solo simulación**. |
| RF-04 | Reproducir la secuencia mediante Tone.js. Cada tick debe:
&nbsp;&nbsp;• resaltar la celda actual (feedback visual)  
&nbsp;&nbsp;• enviar comando `aroma` al Osmo si la celda está activa y la fila habilitada. |
| RF-05 | La UI debe reflejar en tiempo real la aparición/desconexión de Osmos. |

## 4. Diseño de la solución
### 4.1 Navegación
* Se reutilizará `express.static()`; basta con crear `tickertone.html` y sus assets en `src/public/`.
* Añadir barra superior con enlaces a **Estado general** (`/`) y **Ticker-Tone** (`/tickertone`).

### 4.2 Estructura de la página `/tickertone`
```
┌──────────────────────────────────────────────┐
│ Navbar                                       │
├──────────────────────────────────────────────┤
│ Controles globales                           │
│  • Duración (input range 100-5000 ms)        │
│  • BPM (input number)                        │
│  • [ ] Solo simulación                       │
├──────────────────────────────────────────────┤
│ Grid dinámica                                │
│  ╔════════════════════════════════════════╗  │
│  ║ Osmo-123 [Enable] ▢▢▢▢▢▢▢▢              ║  │
│  ║ Osmo-456 [Disable] ▢▢▢▢▢▢▢▢             ║  │
│  ╚════════════════════════════════════════╝  │
└──────────────────────────────────────────────┘
```
* 8 columnas de celdas (`.cell`) corresponden a bombas `0-7`.
* `.cell.active` indica selección; `.cell.playing` indica el paso actual.

### 4.3 Loop de Tone.js
* `Tone.Transport.scheduleRepeat(callback, "8n")`.
* `callback` recorre filas **habilitadas** → celdas activas → `sendCommand()`.
* El parámetro *duración* se toma del slider; se envía en `params.duration`.

### 4.4 Backend
| Endpoint | Método | Descripción |
|----------|--------|-------------|
| `/api/status` | GET | Devuelve `{ mqtt_connected, connected_osmos:[…] }`. |
| `/api/command/:unitId?simulate=bool` | POST | Envía comando al Osmo o simula, según la query. |

Cambios propuestos:
1. **app.js**: Leer `req.query.simulate`; pasarla a `mqttClient.sendCommand()`.
2. **mqttClient.sendCommand()**: Si `simulate===true` → registrar en consola y devolver ID sin publicar.

### 4.5 Modo “Solo simulación”
* El checkbox de la UI agrega `?simulate=true` en el `fetch`.
* Permite probar sin broker activo ni hardware.

## 5. Estrategia de implementación
### 5.1 Front-end
1. Copiar `ticker_tone_test/index.html` a `src/public/tickertone.html` y modularizar en `tickertone.css/js`.
2. Insertar navbar común en `index.html` y `tickertone.html`.
3. Crear función `loadStatus()` para poblar la tabla; llamar cada 2000 ms.
4. Implementar controles globales y almacenar valores en variables reactivas.
5. Adaptar el loop Tone.js para usar `duration` y `simulate`.

### 5.2 Back-end
1. Extender endpoint `/api/command/:unitId` para reconocer `simulate`.
2. Ajustar `mqttClient.sendCommand()` con lógica de bypass.
3. (Opcional) Variable de entorno `SIMULATION_MODE` como override.

### 5.3 Pruebas sugeridas
| Escenario | Descripción |
|-----------|-------------|
| E-01 | Broker activo, simulación **desactivada**: verificar recepción real en Osmos. |
| E-02 | Broker inactivo, simulación **activada**: UI funciona sin errores, consola muestra comandos simulados. |
| E-03 | Cambiar duración y BPM durante la ejecución: observar cambios inmediatos en el disparo. |

## 6. Extensión: Secuenciador 4/4 y Gestión de Aromas

### 6.1 Resumen de requisitos
1. La grilla ahora se basará en compases 4/4 (u otro compás) definidos por el usuario.  
2. El usuario puede seleccionar qué `aroma` (bomba) ejecuta cada celda.  
3. No se permite duplicar un mismo aroma en dos filas simultáneamente.  
4. Botón “Agregar aroma” para insertar filas, con límite de `osmos × 8` filas.

### 6.2 Parámetros configurables
| Parámetro | Descripción | Default |
|-----------|-------------|---------|
| `beatsPerBar` | Nº beats por compás | 4 |
| `subdivisions` | Nº subdivisiones por beat | 4 |
| `totalSteps` | `beatsPerBar × subdivisions` | 16 |
| `maxRows` | `osmosConectados × 8` | 8, 16, 24… |

Los inputs numéricos en la UI actualizan estos valores y disparan `rebuildGrid()`.

### 6.3 Estructura de fila
```
┌───────────┬────────┬─○─○─○─○─○─○─○─○─○─○─○─○─○─○─○─○─┐
│ Osmo ▼    │ Pump ▼ │ • • • • • • • • • • • • • • • • │
└───────────┴────────┴──────────────────────────────────┘
```
* Selector de Osmo (si son >1).  
* Selector de Pump del Osmo (0-7).  
* Toggle Enable/Disable (mute).  
* `totalSteps` celdas.

### 6.4 Algoritmo de unicidad de aromas
1. Mantener `Map<osmoId, Set<pumpId>> usedPumps`.  
2. Al elegir un pump:  
   • Si `pumpId` ya está en la `Set` → bloquear selección y alertar.  
   • Al cambiar o eliminar fila → liberar pump en la `Set`.

### 6.5 Control de filas (Botón “Agregar aroma”)
* Verifica `rows.length < maxRows`.  
* Asigna por defecto: primer Osmo + primer pump libre.  
* Inserta fila en la UI y actualiza estado.

### 6.6 Loop de reproducción (Tone.js)
```
currentStep = (currentStep + 1) % totalSteps
rows.forEach(row => {
  if (!row.enabled) return;
  if (row.pattern[currentStep]) {
    sendCommand(row.osmoId, row.pumpId, duration);
  }
});
```
La visualización marcará el paso actual y cada beat (línea vertical cada `subdivisions`).

### 6.7 Migración paso a paso
1. Añadir controles `beatsPerBar` y `subdivisions` en la barra superior.  
2. Crear estado global `rows` y helpers `addRow`, `validatePump`, `rebuildGrid`.  
3. Adaptar CSS: `grid-template-columns: 180px repeat(var(--steps),40px)`.  
4. Integrar lógica de unicidad y límites.  
5. Ajustar loop Tone.js y feedback visual.  
6. Pruebas con 1 Osmo (máx 8 filas) y 2 Osmos (máx 16 filas).  
7. Documentar controles en la ayuda.

## 7. Futuras mejoras
* Migrar de polling `/api/status` a WebSocket o Server-Sent Events.  
* Persistir patrones de secuencias (JSON) para cargarlos más tarde.  
* Añadir autenticación y control de roles para producción.  
* Dashboard de métricas (tasa de mensajes, latencia, errores).


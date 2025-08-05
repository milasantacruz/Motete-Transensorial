# Timeline Progress - Composición Temporal de Aromas

## 1. Objetivo
Crear una interfaz de línea de tiempo que permita:
1. Componer secuencias de aromas con precisión temporal (segundos)
2. Visualización clara de eventos a lo largo del tiempo
3. Controles de reproducción (play/pause/seek)
4. Gestión avanzada de aromas y eventos

## 2. Arquitectura de la interfaz

### 2.1 Estructura visual
```
┌─────────────────────────────────────────────────────────────┐
│ Navbar: Estado | Ticker-Tone | Piano | Timeline             │
├─────────────────────────────────────────────────────────────┤
│ Controles de reproducción:                                   │
│ Duración: [120] seg | [⏸️] [▶️] [⏹️] | 00:45 / 02:00      │
│ Scrubber: [====█══════════════════════════════════════]     │
├─────────────────────────────────────────────────────────────┤
│ Gestión de aromas:                                          │
│ ┌─ Aroma 1: [Osmo Norte ▼][Bomba 3 ▼][✓ On ][🗑️] ────────┐ │
│ ┌─ Aroma 2: [Osmo Sur   ▼][Bomba 1 ▼][✗ Off][🗑️] ────────┐ │
│ [➕ Agregar Aroma]                                          │
├─────────────────────────────────────────────────────────────┤
│ Timeline canvas:                                            │
│ Aroma 1 ░░█░░░░█░░█░░░░░█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   │
│ Aroma 2 ░░░░█░░░░░░█░░░░░░█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   │
│         |10s|20s|30s|40s|50s|60s|70s|80s|90s|100s|110s|120s│
│ Cursor: ↑ (00:45)                                           │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Componentes principales

#### A. Controles de reproducción
- **Duración total**: Input numérico (5-300 segundos)
- **Play/Pause/Stop**: Botones de control
- **Scrubber**: Barra deslizante para navegación temporal
- **Display temporal**: Tiempo actual / tiempo total

#### B. Gestión de aromas
- **Lista de aromas**: Cada entrada con selector de Osmo, bomba, estado on/off
- **Validación de unicidad**: No duplicar bombas por Osmo
- **Botones**: Agregar/eliminar aromas
- **Estado visual**: Indicar aromas activos/inactivos

#### C. Timeline (línea de tiempo)
- **Canvas/SVG**: Renderizado de la línea temporal
- **Filas por aroma**: Una fila horizontal por cada aroma configurado
- **Marcadores temporales**: Graduación en segundos (10s, 20s, 30s...)
- **Eventos**: Puntos/barras que indican cuándo se activa cada aroma
- **Cursor de reproducción**: Línea vertical que indica la posición actual

## 3. Modelo de datos

### 3.1 Estado global
```javascript
const timelineState = {
  // Configuración temporal
  totalDuration: 120,        // segundos
  currentTime: 0,            // posición actual
  isPlaying: false,          // estado reproducción
  
  // Aromas configurados
  aromas: [
    {
      id: 1,
      name: "Aroma Lavanda",   // nombre personalizable
      osmoId: "osmo_norte",
      pumpId: 3,
      active: true,            // si está habilitado
      color: "#FF6B6B"         // color visual en timeline
    }
  ],
  
  // Eventos en el tiempo
  events: [
    {
      id: 1,
      aromaId: 1,             // referencia al aroma
      time: 15.5,             // segundo exacto
      duration: 2.0           // duración del disparo
    }
  ],
  
  // Configuración de vista
  zoom: 1.0,                 // factor de zoom
  scrollX: 0                 // desplazamiento horizontal
};
```

### 3.2 Validaciones
- **Duración**: Entre 5 y 300 segundos
- **Aromas únicos**: No duplicar (osmoId + pumpId)
- **Eventos válidos**: Tiempo dentro de [0, totalDuration]
- **Límite de aromas**: osmos_conectados × 8

## 4. Funcionalidades por componente

### 4.1 Controles de reproducción
| Funcionalidad | Descripción |
|---------------|-------------|
| **Play** | Inicia reproducción desde currentTime |
| **Pause** | Pausa en la posición actual |
| **Stop** | Detiene y vuelve a 0 |
| **Seek** | Desplaza a una posición específica |
| **Scrubber** | Arrastra para navegar temporalmente |

### 4.2 Gestión de aromas
| Funcionalidad | Descripción |
|---------------|-------------|
| **Agregar** | Crea nuevo aroma con primer osmo/bomba libre |
| **Eliminar** | Borra aroma y todos sus eventos |
| **Cambiar osmo** | Reasigna a otro osmo disponible |
| **Cambiar bomba** | Valida que no esté ocupada |
| **Toggle activo** | Habilita/deshabilita aroma |
| **Cambiar nombre** | Edición in-place del nombre |

### 4.3 Timeline (interacción)
| Acción | Resultado |
|--------|-----------|
| **Click en fila** | Agrega evento en esa posición temporal |
| **Arrastrar evento** | Cambia su posición en el tiempo |
| **Double-click evento** | Elimina el evento |
| **Scroll vertical** | Desplaza entre aromas (si hay muchos) |
| **Scroll horizontal** | Navega en el tiempo |
| **Zoom (Ctrl+scroll)** | Aumenta/reduce resolución temporal |

## 5. Timeline - Especificaciones técnicas

### 5.1 Renderizado
- **Tecnología**: Canvas HTML5 (para performance) o SVG (para interactividad)
- **Dimensiones**: Ancho = viewport - 40px, Alto = 60px × num_aromas + 40px
- **Resolución temporal**: 1 segundo = 20-100 píxeles (según zoom)

### 5.2 Elementos visuales
```
Graduación temporal:
|----|----|----|----|----|----|  (cada 10s línea mayor)
 5s  10s  15s  20s  25s  30s

Fila de aroma:
[Nombre Aroma] ░░█░░█░░░█░░░░░░░  (eventos como rectángulos)

Cursor de reproducción:
                ↓
               █ █  (línea roja vertical)
```

### 5.3 Colores y estados
- **Fondo timeline**: #f8f9fa
- **Graduación**: #dee2e6
- **Evento activo**: Color único por aroma (#FF6B6B, #4ECDC4, #45B7D1...)
- **Evento inactivo**: Mismo color pero 50% opacidad
- **Cursor reproducción**: #dc3545
- **Hover/selección**: Borde amarillo

## 6. Integración con backend

### 6.1 Reproducción
- **Motor**: Tone.js Transport con callbacks programados
- **Precisión**: Usar `Tone.Transport.scheduleOnce()` por cada evento
- **Comandos**: Reutilizar `/api/command/:unitId` con params de duración

### 6.2 Sincronización
```javascript
// Al iniciar reproducción
events.forEach(event => {
  Tone.Transport.scheduleOnce((time) => {
    if (aromas[event.aromaId].active) {
      sendCommand(aroma.osmoId, aroma.pumpId, event.duration);
    }
  }, event.time);
});
```

### 6.3 Estado Osmos
- **Polling**: Mantener `/api/status` cada 2s para detectar desconexiones
- **Validación**: Deshabilitar aromas cuyos osmos estén offline
- **UI**: Marcar visualmente aromas no disponibles

## 7. Plan de implementación

### Fase 1: Estructura base
1. Crear `timeline.html`, `timeline.css`, `timeline.js`
2. Navbar con enlace a Timeline
3. Layout básico con 3 secciones principales
4. Controles de reproducción (sin funcionalidad)

### Fase 2: Gestión de aromas
1. Reutilizar lógica de unicidad de ticker-tone
2. CRUD de aromas con validaciones
3. Integración con polling de osmos
4. Estados activo/inactivo

### Fase 3: Timeline visual
1. Canvas/SVG con graduación temporal
2. Renderizado de filas por aroma
3. Cursor de reproducción animado
4. Zoom y scroll básicos

### Fase 4: Interactividad
1. Click para agregar eventos
2. Drag & drop de eventos existentes
3. Scrubber funcional
4. Controles de zoom

### Fase 5: Reproducción
1. Integración con Tone.js
2. Programación de eventos temporales
3. Sincronización con backend MQTT
4. Modo simulación

### Fase 6: Persistencia (futuro)
1. Guardar/cargar composiciones en JSON
2. Exportar/importar patrones
3. Templates predefinidos

## 8. Casos de uso típicos

### 8.1 Composición básica
1. Usuario define duración total (ej: 60s)
2. Agrega 3 aromas diferentes
3. Hace clic en timeline para colocar eventos:
   - Aroma 1 en 5s, 15s, 25s
   - Aroma 2 en 10s, 30s
   - Aroma 3 en 20s, 40s
4. Presiona Play para probar la composición

### 8.2 Edición avanzada
1. Usuario carga composición existente
2. Cambia zoom para ver más detalle
3. Arrastra eventos para ajustar timing
4. Desactiva temporalmente un aroma
5. Agrega eventos precisos usando scrubber

### 8.3 Producción
1. Usuario finaliza composición
2. Conecta osmos reales
3. Desactiva modo simulación
4. Ejecuta composición completa
5. Observa retroalimentación de LEDs en dispositivos

## 9. Consideraciones técnicas

### 9.1 Performance
- **Canvas**: Para timeline con muchos eventos (>100)
- **Throttling**: Limitar redraws durante scroll/zoom
- **Lazy loading**: Solo renderizar eventos visibles

### 9.2 Usabilidad
- **Shortcuts**: Espacio = play/pause, flechas = seek
- **Visual feedback**: Hover states, selección clara
- **Mobile**: Adaptar controles para touch (fase futura)

### 9.3 Extensibilidad
- **Plugins**: Arquitectura para agregar nuevos tipos de eventos
- **Temas**: Sistema de colores personalizable
- **Exportación**: Formatos estándar (MIDI, JSON)

mosquitto_sub -h localhost -t "motete/#" -u "director" -P "director" -v

# Manejo de Topics MQTT en `local-test`

Este documento detalla la estructura de los mensajes y la lógica de manejo para los topics MQTT suscritos por el cliente director en el entorno de pruebas `local-test`.

---

## 1. Topic de Acciones: `motete/osmo/+/actions`

Este topic se utiliza para que las unidades Osmo confirmen la ejecución de los comandos enviados por el director.

### Estructura del Mensaje (JSON)

```json
{
  "timestamp": "2025-01-14T10:30:01.500Z",
  "command_id": "cmd_001234",
  "action": "aroma",
  "status": "completed",
  "execution_time": 1450,
  "params_executed": {
    "pump": 2,
    "duration": 3000,
    "intensity": 80
  }
}
```

### Lógica de Manejo

-   Al recibir un mensaje, el `mqttClient` registrará la confirmación en la consola.
-   Se podría implementar un sistema de seguimiento de `command_id` para verificar que todos los comandos son confirmados.
-   Los datos pueden ser reenviados a través de WebSockets a una interfaz de monitoreo.

---

## 2. Topic de Errores: `motete/osmo/+/errors`

Este topic es crucial para el monitoreo y la fiabilidad. Las unidades Osmo reportan cualquier fallo o condición inesperada a través de este canal.

### Estructura del Mensaje (JSON)

```json
{
  "timestamp": "2025-01-14T10:30:05.000Z",
  "error_id": "err_5678",
  "command_id": "cmd_001234",
  "unit_id": "osmo_norte",
  "code": "PUMP_MALFUNCTION",
  "message": "La bomba 2 no responde después de 3 intentos."
}
```

### Lógica de Manejo

-   El `mqttClient` registrará el error en la consola con un nivel de severidad `ERROR`.
-   Se generará una alerta que podría ser enviada a la interfaz web.
-   El sistema podría intentar poner a la unidad `unit_id` en modo mantenimiento o pausar la partitura si el error es crítico.

---

## 3. Topic de Sensores: `motete/osmo/+/sensors`

Aunque no está implementado actualmente, este topic se usaría para recibir datos de telemetría de alta frecuencia de los sensores de cada Osmo. La información de estado (`/status`) se usaría para datos de baja frecuencia.

### Estructura del Mensaje (JSON)

```json
{
  "timestamp": "2025-01-14T10:30:02.000Z",
  "unit_id": "osmo_norte",
  "temperature": 24.5,
  "humidity": 45.2,
  "pressure": 1012.5,
  "wifi_strength": -55
}
```

### Lógica de Manejo (Futura)

-   Los datos serían almacenados en una base de datos de series temporales (ej. InfluxDB) para su posterior análisis.
-   Se podrían visualizar en tiempo real en la interfaz de monitoreo.
-   El sistema podría reaccionar a umbrales, por ejemplo, si la temperatura excede un límite seguro. 
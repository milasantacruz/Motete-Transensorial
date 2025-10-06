# Plantilla AWS IoT Core

Esta versión de la plantilla modular está configurada para funcionar con AWS IoT Core.

## 🔧 Cambios Realizados

### 1. **Configuración AWS IoT Core**
- Agregada estructura `AWSConfig` en `config.h`
- Configuración de certificados X.509 en `config.cpp`
- Endpoint de AWS IoT Core configurado

### 2. **NetworkManager Actualizado**
- Cambio de `WiFiClient` a `WiFiClientSecure`
- Configuración automática de certificados
- Topics de AWS IoT Core implementados
- Conexión sin usuario/contraseña (solo certificados)

### 3. **Topics AWS IoT Core**
- **Shadow Update**: `$aws/things/{thingName}/shadow/update`
- **Shadow Delta**: `$aws/things/{thingName}/shadow/update/delta`
- **Comandos**: `motete/director/commands/{unitId}`
- **Respuestas**: `$aws/things/{thingName}/response`
- **Errores**: `$aws/things/{thingName}/errors`

## 📋 Pasos para Configurar

### 1. **Crear Thing en AWS IoT Core**
1. Ir a AWS IoT Core Console
2. Crear un nuevo Thing llamado `osmo_norte`
3. Generar certificados automáticamente

### 2. **Obtener Certificados**
1. Descargar:
   - Certificado del dispositivo (.crt)
   - Clave privada (.key)
   - Certificado CA de AWS (.crt)

### 3. **Configurar en config.cpp**
```cpp
AWSConfig awsConfig = {
    .endpoint = "TU_ENDPOINT_AQUI-ats.iot.us-east-1.amazonaws.com",
    .port = 8883,
    .thingName = "osmo_norte",
    .caCert = R"EOF(
-----BEGIN CERTIFICATE-----
[TU_CERTIFICADO_CA_AQUI]
-----END CERTIFICATE-----
)EOF",
    .deviceCert = R"EOF(
-----BEGIN CERTIFICATE-----
[TU_CERTIFICADO_DISPOSITIVO_AQUI]
-----END CERTIFICATE-----
)EOF",
    .privateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
[TU_CLAVE_PRIVADA_AQUI]
-----END RSA PRIVATE KEY-----
)EOF",
    .qos = 1,
    .keepAlive = 60,
    .cleanSession = true
};
```

### 4. **Crear Política AWS IoT Core**
```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Connect",
        "iot:Publish",
        "iot:Subscribe",
        "iot:Receive"
      ],
      "Resource": [
        "arn:aws:iot:us-east-1:123456789012:client/osmo_norte",
        "arn:aws:iot:us-east-1:123456789012:topic/motete/*",
        "arn:aws:iot:us-east-1:123456789012:topicfilter/motete/*",
        "arn:aws:iot:us-east-1:123456789012:topic/$aws/things/osmo_norte/*"
      ]
    }
  ]
}
```

### 5. **Adjuntar Política al Certificado**
1. En AWS IoT Core Console
2. Ir a Security > Certificates
3. Seleccionar el certificado del dispositivo
4. Adjuntar la política creada

## 🚀 Funcionalidades

### **AWS IoT Core Shadow**
- Estado del dispositivo se publica en Device Shadow
- Comandos pueden llegar via Shadow Delta
- Sincronización automática de estado

### **Topics Personalizados**
- Mantiene compatibilidad con topics `motete/*`
- Comandos del director siguen funcionando
- Respuestas y errores en topics AWS

### **Seguridad**
- Autenticación por certificados X.509
- Comunicación encriptada TLS 1.2
- Sin credenciales de usuario/contraseña

## 🔍 Debugging

### **Logs Importantes**
- `✅ AWS IoT Core conectado` - Conexión exitosa
- `❌ AWS IoT Core falló, rc=X` - Error de conexión
- `✅ Estado publicado en AWS IoT Core Shadow` - Estado enviado

### **Códigos de Error MQTT**
- `-2`: Network timeout
- `-1`: Connection lost
- `0`: Connection successful
- `1-5`: Errores de protocolo MQTT

## ⚠️ Notas Importantes

1. **Endpoint**: Cambiar por tu endpoint real de AWS IoT Core
2. **Certificados**: Reemplazar con tus certificados reales
3. **Región**: Ajustar región en endpoint si es necesario
4. **Memoria**: Los certificados ocupan memoria adicional
5. **Timeout**: Aumentado a 30 segundos para conexión AWS

## 📚 Recursos

- [AWS IoT Core Developer Guide](https://docs.aws.amazon.com/iot/latest/developerguide/)
- [ESP8266 WiFiClientSecure](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)

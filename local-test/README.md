# Pasos para reroducir el Test
## Instalar Node
https://nodejs.org/en/download

## Instalar Mosquitto
https://mosquitto.org/download/

## Instalacion del proyecto node
1. cd local-test
2. cd src
3. npm install

## Iniciar el Sistema
1. Actualizar el archivo de configuracion `mosquitto.conf`: Las rutas de los archivos deben usar `slashes` (`/`) incluso en Windows, ya que así lo requiere Mosquitto. Reemplaza `F:/Persona/Correspondencias Transmodales/local-test` con la ruta absoluta donde creaste la carpeta `local-test`.
2.  **Abre una primera terminal** y navega a la carpeta `config` de tu proyecto. Para Inicia el broker Mosquitto Ejecutar:
    ```bash
        mosquitto -c mosquitto.conf -v
        # La opción -v (verbose) te mostrará los logs en tiempo real.
    ```
    Si ves un error sobre `password_file`, asegúrate de que la ruta en `mosquitto.conf` es **absoluta y correcta**.
3. **Abre una segunda terminal** y navega a la carpeta `src`. Inicia la aplicación Node.js.
    ```bash
    cd src
    node app.js
    ```
    Deberías ver los mensajes "Director conectado al broker MQTT" y "Servidor web escuchando...".
    **Abre tu navegador** y ve a `http://localhost:3000`. Deberías ver la interfaz web con el estado "MQTT: ✅ Conectado" y "Esperando Osmos...".

## Simular un Osmo
1.  **Abre una tercera terminal**. Vamos a simular que `osmo_norte` se conecta y envía su estado.
2.  Usa `mosquitto_pub` para enviar un mensaje de estado. **Las contraseñas estan en el archivo .env. En el archivo passwd se encuentran los usuarios y sus passwd encriptados. los diferentes mensajes de prueba estan en el `local-test/simlation/...**.
3. Asegurate de estar en el directorio local-test. Corre el comando:
```bash
mosquitto_pub -h localhost -t "motete/osmo/osmo_este/status" -u "osmo_este" -P "este" -f ./simulation/osmo_este.json
```

Usuario hace clic en botón
    ↓
JavaScript cambia clase CSS (vivo1/muerto)
    ↓
XMLHttpRequest GET a /vivo1
    ↓
ESP8266 recibe petición
    ↓
funciónEncender1() ejecuta:
    - digitalWrite(aroma1, HIGH)
    - Guarda tiempo de inicio
    - Responde "Emitiendo AROMA 1"
    ↓
JavaScript inicia contador regresivo
    ↓
Después del tiempo configurado:
    - JavaScript cambia clase a "muerto"
    - Envía GET a /muerto1
    - ESP8266 apaga el aroma
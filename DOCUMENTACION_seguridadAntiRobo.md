# Documentación: seguridadAntiRobo.ino

Este documento describe el funcionamiento y la estructura del archivo `seguridadAntiRobo.ino`, un proyecto de Arduino para un sistema de seguridad antirrobo con conectividad WiFi, GPS y Firebase.

---

## Tabla de Contenidos
- [Descripción General](#descripción-general)
- [Dependencias](#dependencias)
- [Definición de Credenciales y Configuración](#definición-de-credenciales-y-configuración)
- [Inicialización de Pines y Objetos](#inicialización-de-pines-y-objetos)
- [setup()](#setup)
- [loop()](#loop)
- [Función vibracionDetectada()](#función-vibraciondetectada)

---

## Descripción General

Este programa permite detectar vibraciones (posible intento de robo), obtener datos de ubicación GPS y enviar esta información a una base de datos en tiempo real de Firebase. El sistema utiliza un ESP32, un sensor de vibración y un módulo GPS.

## Dependencias

- `WiFi.h`: Permite la conexión WiFi.
- `Firebase_ESP_Client.h`: Cliente para interactuar con Firebase.
- `HardwareSerial.h`: Comunicación serie adicional (para GPS).
- `TinyGPSPlus.h`: Decodifica datos del módulo GPS.

## Definición de Credenciales y Configuración

- **WiFi:**
  - `WIFI_SSID`, `WIFI_PASSWORD`: Ahora se obtienen desde variables de entorno mediante `getenv`. En el futuro, estas credenciales serán traídas dinámicamente desde Firebase.
  - Ejemplo de definición de variables de entorno:
    ```cpp
    #define WIFI_SSID getenv("WIFI_SSID")
    #define WIFI_PASSWORD getenv("WIFI_PASSWORD")
    // En el futuro, estas variables serán traídas dinámicamente desde Firebase
    // export WIFI_SSID="TuSSID"
    // export WIFI_PASSWORD="TuPassword"
    ```
- **Firebase:**
  - `API_KEY`, `DATABASE_URL`: Claves y URL de la base de datos.
  - `USER_EMAIL`, `USER_PASSWORD`: Usuario autenticado en Firebase.

## Inicialización de Pines y Objetos

- `pinSensor`: Pin conectado al sensor de vibración.
- `ledPin`: Pin de un LED indicador.
- `gpsSerial`: Puerto serie para el GPS (GPIO16 como RX).
- Objetos de Firebase y GPS.

## setup()

1. Inicializa la comunicación serie y los pines.
2. Configura el puerto serie para el GPS.
3. Conecta a la red WiFi (con timeout de 10 segundos).
4. Configura y autentica la conexión a Firebase.

## loop()

1. Lee el estado del sensor de vibración y determina si hay vibración.
2. Lee y decodifica datos del GPS si están disponibles.
   - Extrae latitud, longitud, altitud, velocidad y fecha/hora.
   - Envía estos datos a Firebase bajo la ruta del usuario autenticado.
3. Si Firebase está listo:
   - Envía el estado de vibración y un timestamp a la base de datos.
   - Muestra mensajes de éxito o error por consola.
4. Enciende o apaga el LED según la vibración detectada.
5. Espera 800 ms antes de repetir el ciclo.

## Función vibracionDetectada(estado)

- Si el sensor detecta vibración (`estado == 1`):
  - Enciende el LED y retorna `true`.
- Si no hay vibración:
  - Apaga el LED y retorna `false`.

---

## Ejemplo de Estructura en Firebase

```
/UsersData/{uid}/gps/
    latitud
    longitud
    altitud
    velocidad
    fechaHoraUTC
/UsersData/{uid}/sensor/
    vibracion
    ultimaVez (timestamp)
```

---

## Notas de Seguridad
- No se recomienda dejar credenciales sensibles (WiFi, API keys, contraseñas) en el código fuente para producción.
- Utilizar variables de entorno o mecanismos seguros para gestionar credenciales.
- En el futuro, las credenciales Wi-Fi podrán ser gestionadas y traídas desde Firebase para mayor seguridad y flexibilidad.

---

## Autor
- Javier Ordoñez Barra
- Email: javier.ordonez.barra@gmail.com 
# Documentación: Sistema de Seguridad Anti-Robo Vehicular

Este documento describe el funcionamiento y la estructura del archivo `seguridadAntiRobo.ino`, un proyecto de ESP32 para un sistema de seguridad antirrobo vehicular con conectividad WiFi, GPS, Firebase y control de servomotor para bloqueo de frenos.

---

## Tabla de Contenidos
- [Descripción General](#descripción-general)
- [Características Principales](#características-principales)
- [Dependencias](#dependencias)
- [Configuración de Firebase](#configuración-de-firebase)
- [Sistema de Credenciales WiFi Dinámico](#sistema-de-credenciales-wifi-dinámico)
- [Hardware Requerido](#hardware-requerido)
- [Configuración de Pines](#configuración-de-pines)
- [Funcionamiento del Sistema](#funcionamiento-del-sistema)
- [Estructura de Datos en Firebase](#estructura-de-datos-en-firebase)
- [Notas de Seguridad](#notas-de-seguridad)

---

## Descripción General

Este sistema de seguridad antirrobo vehicular utiliza un ESP32 para detectar vibraciones (posibles intentos de robo), obtener datos de ubicación GPS en tiempo real, y activar un mecanismo de bloqueo de frenos mediante un servomotor. Toda la información se transmite a Firebase para monitoreo remoto.

## Características Principales

- **Detección de Vibraciones**: Sensor de vibración conectado al vehículo
- **Seguimiento GPS**: Ubicación, velocidad, altitud y timestamp en tiempo real
- **Sistema de Bloqueo**: Servomotor que activa bloqueo de frenos por 5 segundos
- **Conectividad WiFi Dinámica**: Credenciales WiFi obtenidas desde Firebase
- **Monitoreo Remoto**: Datos enviados a Firebase para seguimiento desde aplicación móvil
- **Indicadores Visuales**: LED que indica estado de vibración

## Dependencias

```cpp
#include <WiFi.h>                    // Conexión WiFi
#include <Firebase_ESP_Client.h>      // Cliente Firebase
#include <HardwareSerial.h>           // Comunicación serie adicional
#include <TinyGPSPlus.h>              // Decodificación GPS
#include <Preferences.h>              // Almacenamiento local
#include <ESP32Servo.h>               // Control de servomotor
```

## Configuración de Firebase

```cpp
#define API_KEY "AIzaSyA1bqkhq6z446SbqKJRgqE4j-xOy5vyKGo"
#define DATABASE_URL "https://seguridadantirobovehiculo-default-rtdb.firebaseio.com"
#define USER_EMAIL "javier.ordonez.barra@gmail.com"
#define USER_PASSWORD "1256347xd"
```

## Sistema de Credenciales WiFi Dinámico

El sistema utiliza un enfoque de dos fases para la conexión WiFi:

### Fase 1: Red Temporal
- Se conecta inicialmente a una red WiFi temporal configurada en el código
- Permite acceso a Firebase para obtener las credenciales reales

### Fase 2: Red Configurada
- Obtiene credenciales WiFi desde Firebase (`/UsersData/{uid}/wifi/`)
- Almacena credenciales en memoria interna (Preferences)
- Se reconecta a la red configurada desde la aplicación móvil
- Renueva el token de Firebase después del cambio de red

## Hardware Requerido

- **ESP32**: Microcontrolador principal
- **Sensor de Vibración**: Detecta movimientos sospechosos
- **Módulo GPS**: Proporciona ubicación y datos de movimiento
- **Servomotor**: Activa mecanismo de bloqueo de frenos
- **LED Indicador**: Muestra estado de vibración
- **Fuente de Alimentación**: 5V para servomotor (fuente externa recomendada)

## Configuración de Pines

```cpp
const int pinSensor = 15;     // Sensor de vibración
const int ledPin = 2;         // LED indicador
const int servoPin = 13;      // Servomotor
// GPS: RX=16, TX=17
```

## Funcionamiento del Sistema

### Inicialización (setup())
1. **Configuración de Hardware**: Inicializa pines, servomotor, GPS
2. **Prueba de Servomotor**: Gira a 0°, 180°, 90° para verificar funcionamiento
3. **Conexión WiFi Temporal**: Conecta a red temporal para acceso a Firebase
4. **Autenticación Firebase**: Configura y autentica conexión
5. **Obtención de Credenciales**: Lee credenciales WiFi desde Firebase
6. **Reconexión WiFi**: Cambia a red configurada desde la aplicación
7. **Renovación de Token**: Regenera token de Firebase después del cambio

### Ciclo Principal (loop())
1. **Lectura de Sensores**:
   - Lee estado del sensor de vibración
   - Procesa datos GPS si están disponibles

2. **Envío de Datos GPS**:
   - Latitud, longitud, altitud, velocidad
   - Fecha y hora UTC formateada
   - Envía a Firebase bajo ruta del usuario

3. **Control de Vibración**:
   - Envía estado de vibración a Firebase
   - Actualiza timestamp de última detección
   - Controla LED indicador

4. **Sistema de Bloqueo**:
   - **Activación**: Si detecta vibración y no está activado
     - Gira servomotor a 180°
     - Envía mensaje de alerta a Firebase
     - Activa temporizador de 5 segundos
   - **Desactivación**: Después de 5 segundos
     - Vuelve servomotor a 0°
     - Restaura mensaje de sistema libre
     - Permite nueva activación

### Variables de Control del Servomotor
```cpp
bool estaActivado = false;           // Estado actual del bloqueo
unsigned long tiempoActivado = 0;    // Momento de activación
```

## Estructura de Datos en Firebase

```
/UsersData/{uid}/
├── gps/
│   ├── latitud (double)
│   ├── longitud (double)
│   ├── altitud (double)
│   ├── velocidad (double)
│   └── fechaHoraUTC (string)
├── sensor/
│   ├── vibracion (boolean)
│   ├── ultimaVez (timestamp)
│   └── estadoBloqueo (string)
└── wifi/
    ├── ssid (string)
    └── password (string)
```

### Mensajes de Estado del Bloqueo
- **Activado**: "🔴 Alerta: bloqueo de frenos activado por intento de intrusión"
- **Libre**: "🟢 Sistema libre: sin bloqueo de frenos activo"

## Funciones Auxiliares

### vibracionDetectada(estado)
- Procesa señal del sensor de vibración
- Controla LED indicador
- Retorna boolean indicando presencia de vibración

### obtenerCredencialesWiFi(ssid, password)
- Lee credenciales WiFi desde Firebase
- Retorna true si la operación es exitosa
- Maneja errores de conexión a Firebase

## Notas de Seguridad

### Credenciales
- Las credenciales de Firebase están hardcodeadas (no recomendado para producción)
- El sistema de credenciales WiFi dinámico mejora la seguridad
- Considerar implementar autenticación más robusta

### Alimentación del Servomotor
- **Recomendado**: Fuente externa de 5V con capacidad de 1A o más
- **Evitar**: Alimentar desde pin VIN del ESP32 para servos grandes
- **Importante**: GND común entre ESP32 y fuente del servo

### Configuración de Red
- Red temporal configurada en código para acceso inicial
- Credenciales reales obtenidas dinámicamente desde Firebase
- Sistema de reconexión automática con renovación de tokens

## Solución de Problemas

### Servomotor No Gira
1. **Verificar alimentación**: Fuente de 5V con suficiente corriente
2. **Conexiones**: GND común, señal en pin 13
3. **Código de prueba**: Usar código mínimo para verificar hardware
4. **Pin alternativo**: Probar con otros pines PWM (12, 14, 27, 26)

### Problemas de Conexión WiFi
1. **Red temporal**: Verificar credenciales en código
2. **Firebase**: Confirmar credenciales y URL correctas
3. **Reconexión**: Verificar proceso de cambio de red

---

## Autor
- **Javier Ordoñez Barra**
- Email: javier.ordonez.barra@gmail.com
- Proyecto: Sistema de Seguridad Anti-Robo Vehicular 
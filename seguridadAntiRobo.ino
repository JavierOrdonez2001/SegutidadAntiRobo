// ============================================================================
// BIBLIOTECAS REQUERIDAS
// ============================================================================
#include <WiFi.h>                    // Conexión WiFi
#include <Firebase_ESP_Client.h>      // Cliente Firebase para ESP32
#include <HardwareSerial.h>           // Comunicación serie adicional
#include <TinyGPSPlus.h>              // Decodificación de datos GPS
#include <Preferences.h>              // Almacenamiento persistente en memoria
#include <ESP32Servo.h>               // Control de servomotor

// ============================================================================
// OBJETOS GLOBALES
// ============================================================================
HardwareSerial gpsSerial(1);          // Puerto serie para GPS (UART1)
TinyGPSPlus gps;                      // Objeto para procesar datos GPS
Preferences preferences;               // Objeto para almacenamiento local
Servo servo;                          // Objeto para control del servomotor



// ============================================================================
// CONFIGURACIÓN DE FIREBASE
// ============================================================================
#define API_KEY "AIzaSyA1bqkhq6z446SbqKJRgqE4j-xOy5vyKGo"
#define DATABASE_URL "https://seguridadantirobovehiculo-default-rtdb.firebaseio.com"

// Credenciales de usuario registrado en Firebase Authentication
#define USER_EMAIL "javier.ordonez.barra@gmail.com"
#define USER_PASSWORD "1256347xd"

// Objetos de Firebase para comunicación con la base de datos
FirebaseData fbdo;        // Objeto para operaciones de datos
FirebaseAuth auth;         // Objeto para autenticación
FirebaseConfig config;     // Objeto para configuración

// ============================================================================
// CONFIGURACIÓN DE PINES
// ============================================================================
const int pinSensor = 15;  // Pin del sensor de vibración
const int ledPin = 2;      // Pin del LED indicador
const int servoPin = 13;   // Pin de control del servomotor

// ============================================================================
// VARIABLES DE CONTROL DEL SISTEMA
// ============================================================================
bool estaActivado = false;           // Estado del sistema de bloqueo
unsigned long tiempoActivado = 0;    // Timestamp de activación del bloqueo

// ============================================================================
// CONFIGURACIÓN DE RED TEMPORAL
// ============================================================================
// Red WiFi temporal para acceso inicial a Firebase
// Esta red permite obtener las credenciales reales desde la base de datos
const char* TEMP_WIFI_SSID = "Javier";     // 🔁 Reemplaza por tu red WiFi
const char* TEMP_WIFI_PASS = "1234javi";   // 🔁 Reemplaza por tu contraseña

void setup() {
  // ============================================================================
  // INICIALIZACIÓN DE HARDWARE
  // ============================================================================
  Serial.begin(115200);                    // Inicializar comunicación serie
  Serial.println("🚗 Sistema de Seguridad Anti-Robo Vehicular");
  Serial.println("=============================================");
  
  // Configuración del servomotor
  servo.setPeriodHertz(50);                // Frecuencia típica de servos (50Hz)
  servo.attach(servoPin);                  // Conectar servo al pin 13
  
  // Configuración de pines
  pinMode(pinSensor, INPUT);               // Sensor de vibración como entrada
  pinMode(ledPin, OUTPUT);                 // LED indicador como salida
  
  // Configuración del GPS
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // GPS en pines 16(RX), 17(TX)
  delay(1000);                             // Tiempo de estabilización

  // ============================================================================
  // FASE 1: CONEXIÓN A RED TEMPORAL
  // ============================================================================
  Serial.println("🔌 Conectando a red temporal...");
  WiFi.begin(TEMP_WIFI_SSID, TEMP_WIFI_PASS);
  
  // Intentar conexión con timeout de 10 segundos
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    Serial.print(".");
    delay(300);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ No se pudo conectar a red temporal.");
    return;
  }
  Serial.println("\n✅ Conectado a red temporal.");

  // ============================================================================
  // FASE 2: INICIALIZACIÓN DE FIREBASE
  // ============================================================================
  Serial.println("🔥 Configurando Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("⏳ Esperando autenticación de Firebase...");
  while (!Firebase.ready()) {
    delay(100);
  }
  Serial.println("✅ Firebase autenticado correctamente.");

  // ============================================================================
  // FASE 3: OBTENCIÓN DE CREDENCIALES WIFI REALES
  // ============================================================================
  String ssid, password;
  if (obtenerCredencialesWiFi(ssid, password)) {
    Serial.println("🔐 Credenciales WiFi obtenidas desde Firebase:");
    Serial.println("SSID: " + ssid);

    // Guardar credenciales en memoria persistente
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", password);
    preferences.end();

    // ============================================================================
    // FASE 4: RECONEXIÓN A RED CONFIGURADA
    // ============================================================================
    Serial.println("🔄 Cambiando a red configurada...");
    WiFi.disconnect(true);
    delay(1000);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Intentar conexión con timeout de 10 segundos
    unsigned long attempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - attempt < 10000) {
      Serial.print("*");
      delay(300);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Conectado a red configurada desde la aplicación.");

      // ============================================================================
      // FASE 5: RENOVACIÓN DE TOKEN FIREBASE
      // ============================================================================
      Serial.println("🔄 Renovando token de Firebase...");
      auth.token.uid.clear();              // Limpiar UID anterior
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);

      // Esperar regeneración del token con timeout de 10 segundos
      Serial.println("⏳ Esperando nuevo token...");
      unsigned long tokenStart = millis();
      while ((auth.token.uid == "") && (millis() - tokenStart < 10000)) {
        Firebase.refreshToken(&config);
        delay(200);
      }

      if (auth.token.uid != "") {
        Serial.println("✅ Token renovado correctamente.");
      } else {
        Serial.println("❌ No se pudo obtener nuevo token.");
      }

    } else {
      Serial.println("\n❌ No se pudo conectar a la red configurada.");
    }

  } else {
    Serial.println("❌ No se pudieron obtener credenciales desde Firebase.");
  }

  // ============================================================================
  // PRUEBA DE FUNCIONAMIENTO DEL SERVOMOTOR
  // ============================================================================
  Serial.println("🔧 Probando servomotor...");
  Serial.println("🟢 Giro a 0° (posición inicial)");
  servo.write(0);
  delay(2000);

  Serial.println("🟡 Giro a 180° (posición de bloqueo)");
  servo.write(180);
  delay(2000);

  Serial.println("🔵 Giro a 90° (posición neutral)");
  servo.write(90);
  
  Serial.println("✅ Sistema inicializado correctamente.");
  Serial.println("=============================================");
}


void loop() {
  // ============================================================================
  // LECTURA DE SENSORES
  // ============================================================================
  int estado = digitalRead(pinSensor);     // Leer estado del sensor de vibración
  bool vibracion = vibracionDetectada(estado); // Procesar señal de vibración

  // ============================================================================
  // PROCESAMIENTO DE DATOS GPS
  // ============================================================================
  // Leer datos del GPS si están disponibles
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);                         // Decodificar datos GPS
  }

  // Si hay nuevos datos de ubicación GPS, procesarlos y enviar a Firebase
  if (gps.location.isUpdated()) {
    // Extraer datos de ubicación y movimiento
    double latitude = gps.location.lat();   // Latitud
    double longitude = gps.location.lng();  // Longitud
    double altitude = gps.altitude.meters(); // Altitud en metros
    double speed = gps.speed.kmph();        // Velocidad en km/h

    // Extraer datos de fecha y hora UTC
    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    // Ruta en Firebase para datos GPS del usuario actual
    String gpsPath = "/UsersData/" + String(auth.token.uid.c_str()) + "/gps";

    // Enviar datos GPS a Firebase
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/latitud", latitude);
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/longitud", longitude);
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/altitud", altitude);
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/velocidad", speed);

    // Formatear fecha y hora UTC como string
    char fechaHoraUTC[25];
    sprintf(fechaHoraUTC, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    Firebase.RTDB.setString(&fbdo, gpsPath + "/fechaHoraUTC", fechaHoraUTC);

    // Mostrar datos GPS en consola
    Serial.println("---- Datos GPS ----");
    Serial.print("Latitud: "); Serial.println(latitude, 6);
    Serial.print("Longitud: "); Serial.println(longitude, 6);
    Serial.print("Altitud: "); Serial.print(altitude); Serial.println(" m");
    Serial.print("Velocidad: "); Serial.print(speed); Serial.println(" km/h");
    Serial.print("Fecha UTC: "); Serial.println(fechaHoraUTC);
    Serial.println("-------------------");
  }



  // ============================================================================
  // ENVÍO DE DATOS DE SENSOR A FIREBASE
  // ============================================================================
  if (Firebase.ready()) {
    // Ruta segura asociada al UID del usuario autenticado
    String path = "/UsersData/" + String(auth.token.uid.c_str()) + "/sensor";

    // Enviar estado de vibración a Firebase
    if (Firebase.RTDB.setBool(&fbdo, path + "/vibracion", vibracion)) {
      Serial.println("✅ Estado de vibración enviado a Firebase.");
    } else {
      Serial.print("❌ Error al enviar vibración: ");
      Serial.println(fbdo.errorReason());
    }

    // Enviar timestamp de última detección
    if (Firebase.RTDB.setTimestamp(&fbdo, path + "/ultimaVez")) {
      Serial.println("✅ Timestamp de detección enviado.");
    } else {
      Serial.print("❌ Error al enviar timestamp: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // Mostrar estado de vibración en consola
  Serial.println(vibracion ? "🔴 VIBRACIÓN DETECTADA" : "🟢 Sin vibración");


  // ============================================================================
  // SISTEMA DE BLOQUEO DE FRENOS CON SERVOMOTOR
  // ============================================================================
  
  // ACTIVACIÓN DEL BLOQUEO: Si detecta vibración y el sistema no está activado
  if (vibracion && !estaActivado) {
    servo.write(180);                      // Gira servo a posición de bloqueo (180°)
    estaActivado = true;                   // Marca sistema como activado
    tiempoActivado = millis();             // Registra momento de activación
    Serial.println("🔧 Servomotor activado: posición de bloqueo (180°)");

    // Enviar mensaje de alerta a Firebase
    String path = "/UsersData/" + String(auth.token.uid.c_str()) + "/sensor";
    if (Firebase.RTDB.setString(&fbdo, path + "/estadoBloqueo", "🔴 Alerta: bloqueo de frenos activado por intento de intrusión")) {
      Serial.println("✅ Mensaje de alerta enviado a Firebase");
    } else {
      Serial.print("❌ Error al enviar estado de bloqueo: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // DESACTIVACIÓN DEL BLOQUEO: Después de 5 segundos de activación
  if (estaActivado && millis() - tiempoActivado >= 5000) {
    servo.write(0);                        // Vuelve servo a posición inicial (0°)
    estaActivado = false;                  // Marca sistema como desactivado
    Serial.println("🔁 Servomotor desactivado: posición inicial (0°)");

    // Restaurar mensaje de sistema libre en Firebase
    String path = "/UsersData/" + String(auth.token.uid.c_str()) + "/sensor";
    if (Firebase.RTDB.setString(&fbdo, path + "/estadoBloqueo", "🟢 Sistema libre: sin bloqueo de frenos activo")) {
      Serial.println("✅ Mensaje de sistema libre enviado a Firebase");
    } else {
      Serial.print("❌ Error al restaurar estado de bloqueo: ");
      Serial.println(fbdo.errorReason());
    }
  }

  // ============================================================================
  // INFORMACIÓN DE DEPURACIÓN
  // ============================================================================
  Serial.print("Estado del sistema: ");
  Serial.println(estaActivado ? "🔒 BLOQUEADO" : "🔓 LIBRE");
  
  // Pausa entre ciclos de lectura
  delay(800);
}

// ============================================================================
// FUNCIÓN: DETECCIÓN DE VIBRACIÓN
// ============================================================================
bool vibracionDetectada(int estado) {
  if (estado == 1) {
    digitalWrite(ledPin, HIGH);    // Encender LED indicador
    return true;                   // Vibración detectada
  } else {
    digitalWrite(ledPin, LOW);     // Apagar LED indicador
    return false;                  // Sin vibración
  }
}

// ============================================================================
// FUNCIÓN: OBTENCIÓN DE CREDENCIALES WIFI DESDE FIREBASE
// ============================================================================
bool obtenerCredencialesWiFi(String &ssid, String &password){
  // Ruta en Firebase para credenciales WiFi del usuario actual
  String path = "/UsersData/" + String(auth.token.uid.c_str()) + "/wifi";

  // Leer SSID desde Firebase
  if(!Firebase.RTDB.getString(&fbdo, path + "/ssid")){
    Serial.print("❌ Error obteniendo SSID desde Firebase: ");
    Serial.println(fbdo.errorReason());
    return false;
  }
  ssid = fbdo.stringData();

  // Leer contraseña desde Firebase
  if(!Firebase.RTDB.getString(&fbdo, path + "/password")){
    Serial.print("❌ Error obteniendo contraseña desde Firebase: ");
    Serial.println(fbdo.errorReason());
    return false;
  }
  password = fbdo.stringData();

  return true;  // Credenciales obtenidas exitosamente
}
























#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>


HardwareSerial gpsSerial(1);
TinyGPSPlus gps;
// Credenciales Wi-Fi

// Obtiene las credenciales Wi-Fi desde variables de entorno (simulado)
#define WIFI_SSID getenv("WIFI_SSID")  // Definir variable de entorno WIFI_SSID
#define WIFI_PASSWORD getenv("WIFI_PASSWORD")  // Definir variable de entorno WIFI_PASSWORD
// En el futuro, estas variables serán traídas dinámicamente desde Firebase
// Ejemplo de cómo definir variables de entorno en el entorno de desarrollo o plataforma IoT
// export WIFI_SSID="TuSSID"
// export WIFI_PASSWORD="TuPassword"

// Configuración de Firebase
#define API_KEY "AIzaSyA1bqkhq6z446SbqKJRgqE4j-xOy5vyKGo"
#define DATABASE_URL "https://seguridadantirobovehiculo-default-rtdb.firebaseio.com"

// Usuario registrado en Firebase Authentication
#define USER_EMAIL "javier.ordonez.barra@gmail.com"
#define USER_PASSWORD "1256347xd"

// Objetos de Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pines
const int pinSensor = 15;
const int ledPin = 2;

void setup() {
  Serial.begin(115200);
  pinMode(pinSensor, INPUT);
  pinMode(ledPin, OUTPUT);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // RX = GPIO16 (conectado a TX del gps)

  // Conectar a Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    Serial.print(".");
    delay(300);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ ERROR: No se pudo conectar a WiFi.");
  } else {
    Serial.println("\n✅ Conectado a WiFi.");
  }

  // Configuración de Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Autenticación del usuario
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  int estado = digitalRead(pinSensor);
  bool vibracion = vibracionDetectada(estado);

  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
    //Serial.write(c);
  }


  if (gps.location.isUpdated()) {
    double latitude = gps.location.lat();
    double longitude = gps.location.lng();
    double altitude = gps.altitude.meters();
    double speed = gps.speed.kmph();

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    String gpsPath = "/UsersData/" + String(auth.token.uid.c_str()) + "/gps";

    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/latitud", latitude);
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/longitud", longitude);
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/altitud", altitude);
    Firebase.RTDB.setDouble(&fbdo, gpsPath + "/velocidad", speed);

    // Armar fecha y hora UTC como string
    char fechaHoraUTC[25];
    sprintf(fechaHoraUTC, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    Firebase.RTDB.setString(&fbdo, gpsPath + "/fechaHoraUTC", fechaHoraUTC);

    Serial.println("---- Datos GPS ----");
    Serial.print("Latitud: "); Serial.println(latitude, 6);
    Serial.print("Longitud: "); Serial.println(longitude, 6);
    Serial.print("Altitud: "); Serial.print(altitude); Serial.println(" m");
    Serial.print("Velocidad: "); Serial.print(speed); Serial.println(" km/h");
    Serial.print("Fecha UTC: "); Serial.println(fechaHoraUTC);
    Serial.println("-------------------");
  }



  if (Firebase.ready()) {
    // Ruta segura asociada al UID del usuario
    String path = "/UsersData/" + String(auth.token.uid.c_str()) + "/sensor";

    if (Firebase.RTDB.setBool(&fbdo, path + "/vibracion", vibracion)) {
      Serial.println("✅ Vibración enviada.");
    } else {
      Serial.print("❌ Error vibracion: ");
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.setTimestamp(&fbdo, path + "/ultimaVez")) {
      Serial.println("✅ Timestamp enviado.");
    } else {
      Serial.print("❌ Error timestamp: ");
      Serial.println(fbdo.errorReason());
    }
  }

  Serial.println(vibracion ? "Si hay vibracion" : "No hay vibracion");
  delay(800);
}

bool vibracionDetectada(int estado) {
  if (estado == 1) {
    digitalWrite(ledPin, HIGH);
    return true;
  } else {
    digitalWrite(ledPin, LOW);
    return false;
  }
}

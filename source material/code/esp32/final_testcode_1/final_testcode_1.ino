#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>
#include <DHT.h>

// WiFi Credentials
#define WIFI_SSID "Your_WiFi"
#define WIFI_PASSWORD "Your_Password"

// Firebase Config
#define API_KEY "Your_Firebase_API_Key"
#define DATABASE_URL "Your_Firebase_DB_URL"

// Sensor Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ7_PIN 34
#define MQ135_PIN 35
#define RAIN_PIN 32
#define TRIG_PIN 5
#define ECHO_PIN 18

// Objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);

  dht.begin();
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // GPS on UART1
  bmp.begin(0x76);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // Read all sensors
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = bmp.readPressure() / 100.0F;
  int mq7 = analogRead(MQ7_PIN);
  int mq135 = analogRead(MQ135_PIN);
  int rain = analogRead(RAIN_PIN);
  
  // Water level measurement
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float waterLevel = duration * 0.034 / 2;

  // GPS Data
  String gpsLoc = "No Signal";
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read()) && gps.location.isValid()) {
      gpsLoc = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    }
  }

  // Send to Firebase
  FirebaseJson json;
  json.set("/temperature", temp);
  json.set("/humidity", humidity);
  json.set("/pressure", pressure);
  json.set("/mq7", mq7);
  json.set("/mq135", mq135);
  json.set("/rain", rain);
  json.set("/water_level", waterLevel);
  json.set("/gps", gpsLoc);

  if (Firebase.RTDB.update(&fbdo, "/GuardianSense", &json)) { // FIXED HERE
    Serial.println("Data sent!");
  } else {
    Serial.println("Error: " + fbdo.errorReason());
  }

  delay(5000);
}
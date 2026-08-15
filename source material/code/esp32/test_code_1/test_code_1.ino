#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <TinyGPS++.h>

// WiFi Credentials
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASSWORD "your_wifi_password"
#define API_KEY "your_firebase_api_key"
#define DATABASE_URL "your_firebase_database_url"

FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;

// Sensor Pins
#define MQ7_PIN 36       // MQ-7 CO Sensor
#define MQ138_PIN 39     // MQ-138 Air Quality Sensor
#define RAIN_SENSOR 5    // Rain Detection
#define DHTPIN 4         // DHT11/DHT22 Sensor
#define DHTTYPE DHT22    // Change to DHT11 if using DHT11
#define TRIG_PIN 12      // Ultrasonic Sensor Trig
#define ECHO_PIN 14      // Ultrasonic Sensor Echo

// GPS Module
#define RX_PIN 16
#define TX_PIN 17

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // Use Serial1 for GPS

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    gpsSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    dht.begin();
    bmp.begin();
    
    pinMode(MQ7_PIN, INPUT);
    pinMode(MQ138_PIN, INPUT);
    pinMode(RAIN_SENSOR, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected!");

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    Firebase.begin(&config, &auth);
}

float measureWaterLevel() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    float duration = pulseIn(ECHO_PIN, HIGH);
    float distance = (duration * 0.0343) / 2;  // Convert to cm
    return distance;
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float pressure = bmp.readPressure() / 100.0;  // Convert to hPa
    int gasCO = analogRead(MQ7_PIN);
    int airQuality = analogRead(MQ138_PIN);
    int rain = digitalRead(RAIN_SENSOR);
    float waterLevel = measureWaterLevel();
    bool gpsAvailable = false;
    float latitude, longitude;
    
    while (gpsSerial.available() > 0) {
        if (gps.encode(gpsSerial.read())) {
            latitude = gps.location.lat();
            longitude = gps.location.lng();
            gpsAvailable = true;
        }
    }

    // Send data to Firebase
    if (Firebase.ready()) {
        Firebase.setFloat(fbData, "/HydroSOS/Temperature", temperature);
        Firebase.setFloat(fbData, "/HydroSOS/Humidity", humidity);
        Firebase.setFloat(fbData, "/HydroSOS/Pressure", pressure);
        Firebase.setInt(fbData, "/HydroSOS/CO_Level", gasCO);
        Firebase.setInt(fbData, "/HydroSOS/AirQuality", airQuality);
        Firebase.setInt(fbData, "/HydroSOS/Rain", rain);
        Firebase.setFloat(fbData, "/HydroSOS/WaterLevel", waterLevel);
        if (gpsAvailable) {
            Firebase.setFloat(fbData, "/HydroSOS/Latitude", latitude);
            Firebase.setFloat(fbData, "/HydroSOS/Longitude", longitude);
        }
        Serial.println("Data sent to Firebase");
    }

    delay(5000);
}

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <DHT.h>

// Firebase Add-ons
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi Credentials
#define WIFI_SSID "123456"  
#define WIFI_PASSWORD "12345678"  

// Firebase Credentials
#define API_KEY "AIzaSyBAs6KsI17tEhQyPYKUXuzdf60dZ5zopII"  
#define DATABASE_URL "https://guardiansense-2b188-default-rtdb.asia-southeast1.firebasedatabase.app/"  

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Sensor Objects
Adafruit_BMP280 bmp;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // GPS on UART1
DHT dht(4, DHT11);  // DHT11 on Pin 4

// Sensor Pins
#define MQ7_PIN 34
#define MQ135_PIN 35
#define RAIN_SENSOR_PIN 32
#define TRIG_PIN 5
#define ECHO_PIN 18

// Timing Variables
unsigned long previousMillis = 0;
const long interval = 15000;
bool firebaseReady = false;

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
        Serial.print(".");
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi Connection Failed! Restarting...");
        ESP.restart();
    }
}

void initializeFirebase() {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.token_status_callback = tokenStatusCallback;

    Serial.println("Initializing Firebase...");
    
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase Sign-Up Successful!");
        firebaseReady = true;
    } else {
        Serial.printf("Firebase Sign-Up Failed: %s\n", config.signer.signupError.message.c_str());
    }

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void initializeSensors() {
    if (!bmp.begin(0x76)) {
        Serial.println("BMP280 sensor not found! Check wiring.");
        while (1);
    }
    Serial.println("BMP280 Initialized.");
    
    dht.begin();
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
    
    pinMode(MQ7_PIN, INPUT);
    pinMode(MQ135_PIN, INPUT);
    pinMode(RAIN_SENSOR_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

float getUltrasonicDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2;
    return distance;
}

String getTimestamp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Unknown_Time";
    }
    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", &timeinfo);
    return String(buffer);
}

void sendSensorData() {
    if (Firebase.ready() && firebaseReady) {
        float temperature = bmp.readTemperature();
        float pressure = bmp.readPressure() / 100.0F;
        float humidity = dht.readHumidity();
        int mq7Value = analogRead(MQ7_PIN);
        int mq135Value = analogRead(MQ135_PIN);
        int rainValue = analogRead(RAIN_SENSOR_PIN);
        float waterLevel = getUltrasonicDistance();
        
        // GPS Data
        String gpsLocation = "No Fix";
        float latitude = 0.0, longitude = 0.0;
        if (gps.location.isValid()) {
            latitude = gps.location.lat();
            longitude = gps.location.lng();
            gpsLocation = "https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);
        }

        Serial.printf("Temp: %.2f °C, Pressure: %.2f hPa, Humidity: %.2f%%\n", temperature, pressure, humidity);
        Serial.printf("CO Level: %d, Air Quality: %d, Rain Value: %d, Water Level: %.2f cm\n", mq7Value, mq135Value, rainValue, waterLevel);
        Serial.println("GPS Location: " + gpsLocation);

        // Update Real-time Data
        Firebase.RTDB.setFloat(&fbdo, "/RealTimeData/temperature", temperature);
        Firebase.RTDB.setFloat(&fbdo, "/RealTimeData/pressure", pressure);
        Firebase.RTDB.setFloat(&fbdo, "/RealTimeData/humidity", humidity);
        Firebase.RTDB.setInt(&fbdo, "/RealTimeData/co_level", mq7Value);
        Firebase.RTDB.setInt(&fbdo, "/RealTimeData/air_quality", mq135Value);
        Firebase.RTDB.setInt(&fbdo, "/RealTimeData/rain_value", rainValue);
        Firebase.RTDB.setFloat(&fbdo, "/RealTimeData/water_level", waterLevel);
        Firebase.RTDB.setFloat(&fbdo, "/RealTimeData/gps_lat", latitude);
        Firebase.RTDB.setFloat(&fbdo, "/RealTimeData/gps_lon", longitude);

        // Store Historical Data
        String timestamp = getTimestamp();
        Firebase.RTDB.setFloat(&fbdo, "/HistoricalData/" + timestamp + "/temperature", temperature);
        Firebase.RTDB.setFloat(&fbdo, "/HistoricalData/" + timestamp + "/pressure", pressure);
        Firebase.RTDB.setFloat(&fbdo, "/HistoricalData/" + timestamp + "/humidity", humidity);
        Firebase.RTDB.setInt(&fbdo, "/HistoricalData/" + timestamp + "/co_level", mq7Value);
        Firebase.RTDB.setInt(&fbdo, "/HistoricalData/" + timestamp + "/air_quality", mq135Value);
        Firebase.RTDB.setInt(&fbdo, "/HistoricalData/" + timestamp + "/rain_value", rainValue);
        Firebase.RTDB.setFloat(&fbdo, "/HistoricalData/" + timestamp + "/water_level", waterLevel);
        Firebase.RTDB.setFloat(&fbdo, "/HistoricalData/" + timestamp + "/gps_lat", latitude);
        Firebase.RTDB.setFloat(&fbdo, "/HistoricalData/" + timestamp + "/gps_lon", longitude);
    }
}

void loop() {
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    if (millis() - previousMillis > interval) {
        previousMillis = millis();
        sendSensorData();
    }
}

void setup() {
    Serial.begin(115200);
    connectWiFi();
    initializeFirebase();
    initializeSensors();
}

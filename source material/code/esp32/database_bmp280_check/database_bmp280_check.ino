#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "123456"  // Replace with your WiFi SSID
#define WIFI_PASSWORD "12345678"  // Replace with your WiFi Password
#define API_KEY "AIzaSyBAs6KsI17tEhQyPYKUXuzdf60dZ5zopII"  // Replace with your Firebase API Key
#define DATABASE_URL "https://guardiansense-2b188-default-rtdb.asia-southeast1.firebasedatabase.app/"  // Replace with your Firebase Database URL

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Adafruit_BMP280 bmp;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nConnected to WiFi");
    
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase SignUp OK");
        signupOK = true;
    } else {
        Serial.printf("Firebase SignUp Failed: %s\n", config.signer.signupError.message.c_str());
    }
    
    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    if (!bmp.begin(0x76)) {
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
        while (1);
    }
}

void loop() {
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();
        
        float temperature = bmp.readTemperature();
        float pressure = bmp.readPressure() / 100.0F;
        
        Serial.printf("Temperature: %.2f °C\n", temperature);
        Serial.printf("Pressure: %.2f hPa\n", pressure);
        
        if (Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", temperature)) {
            Serial.println("Temperature updated successfully");
        } else {
            Serial.printf("Failed to update temperature: %s\n", fbdo.errorReason().c_str());
        }
        
        if (Firebase.RTDB.setFloat(&fbdo, "/sensor/pressure", pressure)) {
            Serial.println("Pressure updated successfully");
        } else {
            Serial.printf("Failed to update pressure: %s\n", fbdo.errorReason().c_str());
        }
    }
    delay(5000);
}

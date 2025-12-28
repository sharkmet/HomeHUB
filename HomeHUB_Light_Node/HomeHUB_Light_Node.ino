/**
 * ============================================
 * HomeHUB ESP32 Light Sensing Firmware
 * ============================================
 * REQUIRED ARDUINO IDE LIBRARIES:
 * 1. BH1750 by Christopher Laws (v1.3.0+)
 * 2. ArduinoJson by Benoit Blanchon
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>

// ============================================
// CONFIGURATION
// ============================================
#define WIFI_SSID "Netgear 2006"    // Copied from Env Node
#define WIFI_PASSWORD "woaiPDMS59"  // Copied from Env Node
#define RASPBERRY_PI_IP "10.0.0.47" // Copied from Env Node
#define RASPBERRY_PI_PORT 5000
#define DEVICE_NAME "HomeHUB_Light_Node"

// PIN DEFINITIONS - Updated to match actual wiring
#define I2C_SDA_PIN 26
#define I2C_SCL_PIN 25
#define BH1750_ADDRESS 0x23

#define SENSOR_READ_INTERVAL 10000 // Match server reporting interval (10s)

// GLOBAL OBJECTS
BH1750 lightSensor(BH1750_ADDRESS);
unsigned long lastSend = 0;

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void sendData(float lux) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected. Reconnecting...");
    connectWiFi();
  }

  HTTPClient http;
  String url = "http://" + String(RASPBERRY_PI_IP) + ":5000/sensor-data";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["device_name"] = DEVICE_NAME;

  JsonObject s = doc.createNestedObject("sensors");
  s["light"] = lux;

  String jsonString;
  serializeJson(doc, jsonString);

  int responseCode = http.POST(jsonString);
  if (responseCode > 0)
    Serial.printf("Sent Data (Code %d)\n", responseCode);
  else
    Serial.printf("Error Sending: %s\n",
                  http.errorToString(responseCode).c_str());

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n============================================");
  Serial.println("  HomeHUB Light Node Starting...");
  Serial.println("============================================");

  // Initialize I2C with correct pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100); // Allow I2C bus to stabilize

  // Initialize BH1750
  delay(200); // BH1750 needs time after power-on
  if (lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDRESS,
                        &Wire)) {
    Serial.println("BH1750 Light Sensor [OK]");
  } else {
    Serial.println("BH1750 Light Sensor [FAIL]");
    Serial.println("Check wiring: SDA=26, SCL=25, VCC=3.3V, ADDR=GND");
  }

  // Initialize WiFi
  connectWiFi();

  Serial.println("\nHomeHUB Light Node Ready!");
  Serial.println("============================================\n");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSend >= SENSOR_READ_INTERVAL) {
    lastSend = currentMillis;

    float lux = lightSensor.readLightLevel();

    if (lux >= 0) {
      Serial.printf("Light Level: %.1f lux\n", lux);
      sendData(lux);
    } else {
      Serial.println("Failed to read Light sensor!");
    }
  }
}

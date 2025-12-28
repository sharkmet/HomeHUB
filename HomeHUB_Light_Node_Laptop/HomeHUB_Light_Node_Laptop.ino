/**
 * ============================================
 * HomeHUB ESP32 Light Sensing Firmware
 * LAPTOP VERSION - Sends data to Windows PC
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
// CONFIGURATION - LAPTOP VERSION
// ============================================
#define WIFI_SSID "Netgear 2006"
#define WIFI_PASSWORD "woaiPDMS59"
#define SERVER_IP "10.0.0.12" // Your laptop's IP
#define SERVER_PORT 5000
#define DEVICE_NAME "HomeHUB_Light_Node"

// PIN DEFINITIONS - Match your wiring
#define I2C_SDA_PIN 26
#define I2C_SCL_PIN 25
#define BH1750_ADDRESS 0x23

#define SENSOR_READ_INTERVAL 10000 // 10 seconds

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
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void sendData(float lux) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected. Reconnecting...");
    connectWiFi();
  }

  HTTPClient http;
  String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) +
               "/sensor-data";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["device_name"] = DEVICE_NAME;

  JsonObject s = doc.createNestedObject("sensors");
  s["light"] = lux;

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.print("Sending to: ");
  Serial.println(url);

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
  Serial.println("  HomeHUB Light Node - LAPTOP VERSION");
  Serial.println("============================================");
  Serial.print("Target Server: ");
  Serial.print(SERVER_IP);
  Serial.print(":");
  Serial.println(SERVER_PORT);

  // Initialize I2C with correct pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  // Initialize BH1750
  delay(200);
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

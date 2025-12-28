/**
 * ============================================
 * HomeHUB ESP32 Env Node (DHT22 + Mic)
 * LAPTOP VERSION - Sends data to Windows PC
 * ============================================
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <WiFi.h>

// ============================================
// CONFIGURATION - LAPTOP VERSION
// ============================================
#define WIFI_SSID "Netgear 2006"
#define WIFI_PASSWORD "woaiPDMS59"
#define SERVER_IP "10.0.0.12" // Your laptop's IP
#define SERVER_PORT 5000
#define DEVICE_NAME "HomeHUB_Env_Node"

// PINS & SETTINGS
#define MIC_PIN 35
#define DHT_PIN 4
#define DHT_TYPE DHT22

#define AUDIO_SAMPLES 64
#define AUDIO_NOISE_FLOOR 100
#define WIFI_SEND_INTERVAL 10000 // Send data every 10 seconds

// ============================================
// SENSOR CLASSES
// ============================================
class DHTSensor {
private:
  DHT *_dht;
  float _lastTemp;
  float _lastHumidity;
  bool _initialized;

  bool validateReading(float temp, float humidity) {
    if (isnan(temp) || isnan(humidity))
      return false;
    if (temp < -40 || temp > 80)
      return false;
    if (humidity < 0 || humidity > 100)
      return false;
    return true;
  }

public:
  DHTSensor()
      : _dht(nullptr), _lastTemp(0.0f), _lastHumidity(0.0f),
        _initialized(false) {}

  void begin() {
    _dht = new DHT(DHT_PIN, DHT_TYPE);
    _dht->begin();
    delay(2000); // Warmup
    _initialized = true;
  }

  bool read(float &temp, float &hum) {
    if (!_initialized)
      return false;
    float t = _dht->readTemperature();
    float h = _dht->readHumidity();

    if (validateReading(t, h)) {
      temp = _lastTemp = t;
      hum = _lastHumidity = h;
      return true;
    }
    return false;
  }
};

class MicrophoneSensor {
private:
  int _peakLevel;

public:
  MicrophoneSensor() : _peakLevel(0) {}

  void begin() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
  }

  void sample() {
    int minVal = 4095;
    int maxVal = 0;

    for (int i = 0; i < AUDIO_SAMPLES; i++) {
      int val = analogRead(MIC_PIN);
      if (val < minVal)
        minVal = val;
      if (val > maxVal)
        maxVal = val;
    }

    int peakToPeak = maxVal - minVal;

    if (peakToPeak < 10)
      peakToPeak = 0;
    else
      peakToPeak -= 10;

    if (peakToPeak > _peakLevel)
      _peakLevel = peakToPeak;
  }

  int getPeakAndReset() {
    int p = _peakLevel;
    _peakLevel = 0;
    return p;
  }
};

// ============================================
// GLOBAL OBJECTS
// ============================================
DHTSensor dhtSensor;
MicrophoneSensor micSensor;
unsigned long lastSend = 0;
unsigned long lastSample = 0;

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

void sendData(float temp, float hum, int audioPeak) {
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
  s["temperature"] = temp;
  s["humidity"] = hum;
  s["audio_peak"] = audioPeak;

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

// ============================================
// MAIN LOOP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n============================================");
  Serial.println("  HomeHUB Env Node - LAPTOP VERSION");
  Serial.println("============================================");
  Serial.print("Target Server: ");
  Serial.print(SERVER_IP);
  Serial.print(":");
  Serial.println(SERVER_PORT);

  dhtSensor.begin();
  micSensor.begin();
  connectWiFi();

  Serial.println("\nEnv Node Ready!");
  Serial.println("============================================\n");
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. High Frequency Audio Sampling (Every 100ms)
  if (currentMillis - lastSample >= 100) {
    lastSample = currentMillis;
    micSensor.sample();
  }

  // 2. Data Reporting (Every 10s)
  if (currentMillis - lastSend >= WIFI_SEND_INTERVAL) {
    lastSend = currentMillis;

    float t, h;
    if (dhtSensor.read(t, h)) {
      int peak = micSensor.getPeakAndReset();

      Serial.printf("Temp: %.1f C | Hum: %.1f %% | Audio Peak: %d\n", t, h,
                    peak);
      sendData(t, h, peak);
    } else {
      Serial.println("Failed to read DHT sensor!");
    }
  }
}

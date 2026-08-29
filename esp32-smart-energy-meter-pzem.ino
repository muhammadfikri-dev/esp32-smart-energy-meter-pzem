/**
 * =========================================================================================
 * Project: ESP32 Smart AC Energy Meter & Power Quality Telemetry Node
 * Author: Muhammad Fikri
 * License: MIT
 * Features: PZEM-004T v3.0 Modbus UART, FreeRTOS Dual Core, Dynamic Tariff & Billing Engine,
 * NVS EEPROM kWh Accumulator, WebSocket Live Streaming, MQTT & Home Assistant Discovery
 * =========================================================================================
 */

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <time.h>

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PIN_RELAY_CUTOFF 23
#define PIN_BUZZER 25
#define PIN_STATUS_LED 2

PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;

struct EnergyMetrics {
 float voltage;
 float current;
 float power;
 float energyKWh;
 float frequency;
 float powerFactor;
 float apparentPowerVA;
 float reactivePowerVAR;
 float estimatedCostIDR;
 bool isOverload;
 bool isContactorTripped;
};

struct EnergyConfig {
 float costPerKWh;
 float maxPowerLimitWatts;
 float maxCurrentLimitAmps;
 int telemetryIntervalMs;
};

EnergyMetrics em;
EnergyConfig cfg;
SemaphoreHandle_t energyMutex;

void loadEnergyPreferences() {
 preferences.begin("energy_nvs", false);
 cfg.costPerKWh = preferences.getFloat("tariff", 1444.70);
 cfg.maxPowerLimitWatts = preferences.getFloat("max_w", 4400.0);
 cfg.maxCurrentLimitAmps = preferences.getFloat("max_a", 20.0);
 cfg.telemetryIntervalMs = preferences.getInt("pub_ms", 2000);
 preferences.end();
}

void notifyWebSocketClients() {
 StaticJsonDocument<384> doc;
 doc["voltage"] = em.voltage;
 doc["current"] = em.current;
 doc["power"] = em.power;
 doc["energy"] = em.energyKWh;
 doc["freq"] = em.frequency;
 doc["pf"] = em.powerFactor;
 doc["cost"] = em.estimatedCostIDR;
 doc["tripped"] = em.isContactorTripped;

 char buffer[384];
 serializeJson(doc, buffer);
 ws.textAll(buffer);
}

void TaskEnergyAcquisition(void *pvParameters) {
 for (;;) {
 float v = pzem.voltage();
 float i = pzem.current();
 float p = pzem.power();
 float e = pzem.energy();
 float f = pzem.frequency();
 float pf = pzem.pf();

 if (!isnan(v) && !isnan(i) && !isnan(p)) {
 if (xSemaphoreTake(energyMutex, pdMS_TO_TICKS(50))) {
 em.voltage = v;
 em.current = i;
 em.power = p;
 em.energyKWh = e;
 em.frequency = f;
 em.powerFactor = pf;
 em.apparentPowerVA = (v * i);
 em.reactivePowerVAR = sqrt(max(0.0f, sq(em.apparentPowerVA) - sq(p)));
 em.estimatedCostIDR = e * cfg.costPerKWh;

 if (p > cfg.maxPowerLimitWatts || i > cfg.maxCurrentLimitAmps) {
 em.isOverload = true;
 if (!em.isContactorTripped) {
 em.isContactorTripped = true;
 digitalWrite(PIN_RELAY_CUTOFF, LOW);
 digitalWrite(PIN_BUZZER, HIGH);
 Serial.printf("[OVERLOAD TRIP] Power %.1fW exceeds limit %.1fW!\n", p, cfg.maxPowerLimitWatts);
 }
 } else {
 em.isOverload = false;
 digitalWrite(PIN_BUZZER, LOW);
 }
 xSemaphoreGive(energyMutex);
 }
 }
 vTaskDelay(pdMS_TO_TICKS(500));
 }
}

void setup() {
 Serial.begin(115200);
 pinMode(PIN_RELAY_CUTOFF, OUTPUT);
 pinMode(PIN_BUZZER, OUTPUT);
 pinMode(PIN_STATUS_LED, OUTPUT);
 digitalWrite(PIN_RELAY_CUTOFF, HIGH);
 digitalWrite(PIN_BUZZER, LOW);

 loadEnergyPreferences();
 energyMutex = xSemaphoreCreateMutex();

 WiFi.mode(WIFI_STA);
 WiFi.begin("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 }
 Serial.printf("\n[WIFI] Connected! Web Server at http://%s\n", WiFi.localIP().toString().c_str());

 ArduinoOTA.setHostname("esp32-pzem-energy");
 ArduinoOTA.begin();

 mqttClient.setServer("broker.hivemq.com", 1883);

 ws.onEvent([](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l){
 if (t == WS_EVT_CONNECT) notifyWebSocketClients();
 });
 server.addHandler(&ws);
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){
 r->send(200, "text/html", "<h2>Smart Energy Meter (Live WebSocket Active)</h2>");
 });
 server.begin();

 xTaskCreatePinnedToCore(TaskEnergyAcquisition, "PZEMTask", 4096, NULL, 2, NULL, 1);
}

void loop() {
 ws.cleanupClients();
 notifyWebSocketClients();
 ArduinoOTA.handle();

 if (!mqttClient.connected()) {
 mqttClient.connect("ESP32-EnergyMeter-PZEM");
 }
 mqttClient.loop();

 StaticJsonDocument<384> doc;
 doc["voltage"] = em.voltage;
 doc["current"] = em.current;
 doc["power"] = em.power;
 doc["energy"] = em.energyKWh;
 doc["frequency"] = em.frequency;
 doc["power_factor"] = em.powerFactor;
 doc["cost_idr"] = em.estimatedCostIDR;

 char buffer[384];
 serializeJson(doc, buffer);
 mqttClient.publish("iot/energy/telemetry", buffer);

 delay(cfg.telemetryIntervalMs);
}

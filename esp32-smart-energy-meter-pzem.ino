#include <WiFi.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include <ArduinoJson.h>

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17

PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "broker.hivemq.com";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) client.connect("ESP32-Energy-Meter");
  client.loop();

  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  StaticJsonDocument<256> doc;
  doc["voltage_v"] = voltage;
  doc["current_a"] = current;
  doc["power_w"] = power;
  doc["energy_kwh"] = energy;
  doc["pf"] = pf;

  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("laksanasoft/energy/meter_01", buffer);
  Serial.printf("Published Energy: %s\n", buffer);
  delay(5000);
}
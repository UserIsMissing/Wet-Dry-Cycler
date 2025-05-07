#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "HEATING.h"

#define Serial0 Serial

// Wi-Fi credentials
// const char* ssid = "UCSC-Devices";
// const char* password = "o9ANAjrZ9zkjYKy2yL";

const char* ssid = "DonnaHouse";
const char* password = "guessthepassword";

// const char *ssid = "UCSC-Guest";
// const char* password = "";

// GPIO pin definitions
const int LED_PIN = 2;
const int MIX1_GPIO = 11;
const int MIX2_GPIO = 12;
const int MIX3_GPIO = 13;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleGpio(const String &name, const String &state)
{
  int level = (state == "on") ? HIGH : LOW;

  if (name == "led")
    digitalWrite(LED_PIN, level);
  else if (name == "mix1")
    digitalWrite(MIX1_GPIO, level);
  else if (name == "mix2")
    digitalWrite(MIX2_GPIO, level);
  else if (name == "mix3")
    digitalWrite(MIX3_GPIO, level);

  Serial.printf("Set %s to %s\n", name.c_str(), state.c_str());
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial0.printf("WebSocket client #%u connected\n", client->id());
  }

  if (type == WS_EVT_DISCONNECT)
  {
    Serial0.printf("WebSocket client #%u disconnected\n", client->id());
  }

  if (type == WS_EVT_DATA)
  {
    Serial0.printf("Received data (%d bytes): ", len);
    for (size_t i = 0; i < len; i++)
    {
      Serial0.print((char)data[i]);
    }
    Serial0.println();

    StaticJsonDocument<200> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err)
    {
      Serial0.print("JSON parse failed: ");
      Serial0.println(err.c_str());
      return;
    }

    if (doc["name"].is<const char *>() && doc["state"].is<const char *>())
    {
      String name = doc["name"].as<String>();
      String state = doc["state"].as<String>();

      Serial0.printf("Parsed: name = %s, state = %s\n", name.c_str(), state.c_str());

      handleGpio(name, state);
    }
    else
    {
      Serial0.println("Missing keys in JSON");
    }
  }
}

void sendTemperature()
{
  float temp = HEATING_Measure_Temp_Avg();

  StaticJsonDocument<100> doc;
  doc["type"] = "temperature";
  doc["value"] = temp;

  char buffer[100];
  size_t len = serializeJson(doc, buffer);
  ws.textAll(buffer);

  Serial.printf("Sent temp: %.2f °C\n", temp);
}

void setup()
{
  Serial.begin(115200);
  delay(2000); // Allow USB Serial to connect

  // GPIO init
  pinMode(LED_PIN, OUTPUT);
  pinMode(MIX1_GPIO, OUTPUT);
  pinMode(MIX2_GPIO, OUTPUT);
  pinMode(MIX3_GPIO, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(MIX1_GPIO, LOW);
  digitalWrite(MIX2_GPIO, LOW);
  digitalWrite(MIX3_GPIO, LOW);

  HEATING_Init();

  // Wi-Fi connect
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  // WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  Serial0.print("ESP32 MAC Address: ");
  Serial0.println(WiFi.macAddress());

  // WebSocket setup
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("WebSocket server started");
}

unsigned long lastSent = 0;

void loop()
{
  unsigned long now = millis();
  if (now - lastSent >= 1000)
  { // Send temperature every 1s
    sendTemperature();
    lastSent = now;
  }
}

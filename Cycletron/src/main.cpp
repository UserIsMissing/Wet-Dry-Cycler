#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "HEATING.h"
#include "MIXING.h"
#include "REHYDRATION.h"
#include <stdlib.h>  // for atof()

#define Serial0 Serial

// Wi-Fi credentials
// const char* ssid = "UCSC-Devices";
// const char* password = "o9ANAjrZ9zkjYKy2yL";

// const char *ssid = "DonnaHouse";
// const char *password = "guessthepassword";

const char *ssid = "TheDawgHouse";
const char *password = "ThrowItBackForPalestine";

// const char *ssid = "UCSC-Guest";
// const char* password = "";

// === State Machine ===
enum class SystemState
{
  IDLE,
  READY,
  RUNNING,
  REFILLING,
  EXTRACTING,
  LOGGING,
  PAUSED,
  ENDED,
  ERROR
};

// ===  Parameters ===
float volumeAddedPerCycle = 0;
float durationOfRehydration = 0;
float syringeDiameter = 0;
float desiredHeatingTemperature = 0;
float durationOfHeating = 0;
float durationOfMixing = 0;
int numberOfCycles = 0;
std::vector<int> sampleZonesToMix; // If using std::vector (with -fno-rtti)

// Or, use a fixed array if you prefer:
int sampleZonesArray[10];
int sampleZoneCount = 0;

SystemState currentState = SystemState::IDLE;
SystemState previousState = SystemState::IDLE;

// === Web Server & WebSocket ===
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// === State Handling ===
void handleStateCommand(const String &name, const String &state)
{
  // Block all state changes if system is still in IDLE
  if (currentState == SystemState::IDLE)
  {
    Serial.println("System is IDLE — cannot process commands until parameters are received.");
    return;
  }
  // Allow commands only from READY or RUNNING (or related intermediate) states
  if (name == "startCycle" && state == "on")
  {
    currentState = SystemState::RUNNING;
    Serial.println("State changed to RUNNING");
  }
  else if (name == "pauseCycle")
  {
    if (state == "on")
    {
      currentState = SystemState::PAUSED;
      Serial.println("State changed to PAUSED");
    }
    else if (state == "off")
    {
      currentState = SystemState::RUNNING;
      Serial.println("State changed to RESUMED (RUNNING)");
    }
  }
  else if (name == "endCycle" && state == "on")
  {
    currentState = SystemState::ENDED;
    Serial.println("State changed to ENDED");
  }
  else if (name == "extract")
  {
    if (state == "on")
    {
      currentState = SystemState::EXTRACTING;
      Serial.println("State changed to EXTRACTING");
    }
    else if (state == "off")
    {
      currentState = SystemState::RUNNING;
      Serial.println("Extraction ended — resuming RUNNING");
    }
  }
  else if (name == "refill")
  {
    if (state == "on")
    {
      currentState = SystemState::REFILLING;
      Serial.println("State changed to REFILLING");
    }
    else if (state == "off")
    {
      currentState = SystemState::RUNNING;
      Serial.println("Refill ended — resuming RUNNING");
    }
  }
  else if (name == "logCycle" && state == "on")
  {
    previousState = currentState; // Save current state
    currentState = SystemState::LOGGING;
    Serial.println("State changed to LOGGING");
  }
  else
  {
    Serial.printf("Unknown or ignored command: %s with state: %s\n", name.c_str(), state.c_str());
  }
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

    // Handle 'parameters' packet
    if (doc["type"] == "parameters" && doc["data"].is<JsonObject>())
    {
      JsonObject data = doc["data"].as<JsonObject>();

      // Parse floats

      volumeAddedPerCycle = atof(data["volumeAddedPerCycle"] | "0");
      durationOfRehydration = atof(data["durationOfRehydration"] | "0");
      syringeDiameter = atof(data["syringeDiameter"] | "0");
      desiredHeatingTemperature = atof(data["desiredHeatingTemperature"] | "0");
      durationOfHeating = atof(data["durationOfHeating"] | "0");
      durationOfMixing = atof(data["durationOfMixing"] | "0");
      numberOfCycles = atoi(data["numberOfCycles"] | "0"); // use atoi for integer

      // Parse array
      sampleZoneCount = 0;
      if (data["sampleZonesToMix"].is<JsonArray>())
      {
        JsonArray zones = data["sampleZonesToMix"].as<JsonArray>();
        for (JsonVariant val : zones)
        {
          if (val.is<int>() && sampleZoneCount < 10)
          {
            sampleZonesArray[sampleZoneCount++] = val.as<int>();
          }
        }
      }

      // Log for confirmation
      Serial.println("[REHYDRATION] Received all parameters:");
      Serial.printf("  Volume per cycle: %.2f µL\n", volumeAddedPerCycle);
      Serial.printf("  Rehydration duration: %.2f s\n", durationOfRehydration);
      Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
      Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
      Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
      Serial.printf("  Number of cycles: %d\n", numberOfCycles);

      // Initialize modules now that we have real parameters
      Rehydration_Init(syringeDiameter);
      if (currentState == SystemState::IDLE)
      {
        currentState = SystemState::READY;
        Serial.println("Received parameters — transitioning to READY state");
      }
      else
      {
        Serial.println("Received parameters, but system is not in IDLE");
      }
      return;
    }
    // Handle regular control command
    if (doc["name"].is<const char *>() && doc["state"].is<const char *>())
    {
      String name = doc["name"].as<String>();
      String state = doc["state"].as<String>();

      Serial0.printf("Parsed: name = %s, state = %s\n", name.c_str(), state.c_str());

      handleStateCommand(name, state);
      StaticJsonDocument<100> response;
      response["name"] = name;
      response["state"] = state;

      char jsonBuffer[100];
      size_t len = serializeJson(response, jsonBuffer);
      ws.textAll(jsonBuffer);
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

  HEATING_Init();
  MIXING_Init();
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
  Serial.println(WiFi.localIP());
}

unsigned long lastSent = 0;

void loop()
{
  unsigned long now = millis();

  switch (currentState)
  {
  case SystemState::IDLE:
    // Wait for parameter packet
    break;

  case SystemState::READY:
    // Await user input (startCycle)
    break;

  case SystemState::RUNNING:
    // Send temperature periodically
    if (now - lastSent >= 1000)
    {
      sendTemperature();
      lastSent = now;
    }
    break;

  case SystemState::PAUSED:
    // System is paused — do nothing
    break;

  case SystemState::ENDED:
    // System ended — could power down, stop all tasks, etc.
    currentState = SystemState::IDLE;
    break;

  case SystemState::REFILLING:
    Serial.println("Refilling...");
    // Optional: trigger refill hardware here
    currentState = SystemState::PAUSED;
    break;

  case SystemState::EXTRACTING:
    Serial.println("Extracting...");
    // Optional: trigger extract hardware here
    currentState = SystemState::PAUSED;
    break;

  case SystemState::LOGGING:
    Serial.println("Logging data...");
    // [Optional: insert actual logging logic here]
    currentState = previousState; // Resume previous state
    break;

    break;

  case SystemState::ERROR:
    Serial.println("System error — awaiting reset or external command.");
    break;
  }
}
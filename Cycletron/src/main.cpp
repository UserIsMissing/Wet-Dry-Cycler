#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "HEATING.h"
#include "MIXING.h"
#include "REHYDRATION.h"
#include "MOVEMENT.h"

#include <stdlib.h> // for atof()

// TESTS
#define TESTING_MAIN

#define Serial0 Serial
#define ServerIP "169.233.119.230"
#define ServerPort 5175

// === Wi-Fi Credentials ===
// const char* ssid = "UCSC-Devices";
// const char* password = "o9ANAjrZ9zkjYKy2yL";

// const char *ssid = "DonnaHouse";
// const char *password = "guessthepassword";

// const char *ssid = "TheDawgHouse";
// const char *password = "ThrowItBackForPalestine";

const char *ssid = "UCSC-Guest";
const char *password = "";

// === State Machine ===
enum class SystemState
{
  IDLE,
  READY,
  REHYDRATING,
  HEATING,
  MIXING,
  REFILLING,
  EXTRACTING,
  LOGGING,
  PAUSED,
  ENDED,
  ERROR
};

// === Operational Parameters ===
float volumeAddedPerCycle = 0;
float durationOfRehydration = 0;
float syringeDiameter = 0;
float desiredHeatingTemperature = 0;
float durationOfHeating = 0;
float durationOfMixing = 0;
int numberOfCycles = 0;
// std::vector<int> sampleZonesToMix; // If using std::vector (with -fno-rtti)

// Or, use a fixed array if you prefer:
int sampleZonesArray[10];
int sampleZoneCount = 0;

SystemState currentState = SystemState::IDLE;
SystemState previousState = SystemState::IDLE;

// === WebSocket Client ===
WebSocketsClient webSocket;

void setState(SystemState newState)
{
  if (currentState != SystemState::PAUSED &&
      currentState != SystemState::REFILLING &&
      currentState != SystemState::EXTRACTING)
  {
    previousState = currentState;
  }
  currentState = newState;
}

void handleStateCommand(const String &name, const String &state)
{
  if (currentState == SystemState::IDLE)
  {
    Serial.println("System is IDLE — cannot process commands until parameters are received.");
    return;
  }
  if (name == "startCycle" && state == "on")
  {
    setState(SystemState::REHYDRATING);
    Serial.println("State changed to REHYDRATING");
  }
  else if (name == "pauseCycle")
  {
    if (state == "on")
    {
      previousState = currentState;
      currentState = SystemState::PAUSED;
      Serial.println("State changed to PAUSED");
    }
    else
    {
      currentState = previousState;
      Serial.printf("State RESUMED (currentState = %d)", static_cast<int>(currentState));
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
      previousState = currentState;
      currentState = SystemState::EXTRACTING;
      Serial.println("Extraction started");
    }
    else
    {
      currentState = previousState;
      Serial.println("Extraction ended — resuming");
    }
  }
  else if (name == "refill")
  {
    if (state == "on")
    {
      previousState = currentState;
      currentState = SystemState::REFILLING;
      Serial.println("Refill started");
    }
    else
    {
      currentState = previousState;
      Serial.println("Refill ended — resuming");
    }
  }
  else if (name == "logCycle" && state == "on")
  {
    previousState = currentState;
    currentState = SystemState::LOGGING;
    Serial.println("State changed to LOGGING");
  }
  else
  {
    Serial.printf("Unknown or ignored command: %s with state: %s\n", name.c_str(), state.c_str());
  }
}

void handleRecoveryPacket(const JsonObject &data)
{
  if (!data.containsKey("currentState") || !data.containsKey("parameters"))
  {
    Serial.println("Recovery packet is empty or invalid. Transitioning to IDLE state.");
    currentState = SystemState::IDLE;
    return;
  }

  // Restore the last known state
  String recoveredState = data["currentState"].as<String>();
  if (recoveredState == "HEATING")
    currentState = SystemState::HEATING;
  else if (recoveredState == "REHYDRATING")
    currentState = SystemState::REHYDRATING;
  else if (recoveredState == "MIXING")
    currentState = SystemState::MIXING;
  else if (recoveredState == "READY")
    currentState = SystemState::READY;
  else
    currentState = SystemState::IDLE;

  // Restore parameters
  JsonObject parameters = data["parameters"];
  volumeAddedPerCycle = atof(parameters["volumeAddedPerCycle"] | "0");
  durationOfRehydration = atof(parameters["durationOfRehydration"] | "0");
  syringeDiameter = atof(parameters["syringeDiameter"] | "0");
  desiredHeatingTemperature = atof(parameters["desiredHeatingTemperature"] | "0");
  durationOfHeating = atof(parameters["durationOfHeating"] | "0");
  durationOfMixing = atof(parameters["durationOfMixing"] | "0");
  numberOfCycles = atoi(parameters["numberOfCycles"] | "0");

  // Restore sample zones
  sampleZoneCount = 0;
  if (parameters["sampleZonesToMix"].is<JsonArray>())
  {
    JsonArray zones = parameters["sampleZonesToMix"].as<JsonArray>();
    for (JsonVariant val : zones)
    {
      if (val.is<int>() && sampleZoneCount < 10)
      {
        sampleZonesArray[sampleZoneCount++] = val.as<int>();
      }
    }
  }

  // Log recovery details
  Serial.println("[RECOVERY] Restored system state and parameters:");
  Serial.printf("  Current state: %s\n", recoveredState.c_str());
  Serial.printf("  Volume per cycle: %.2f µL\n", volumeAddedPerCycle);
  Serial.printf("  Rehydration duration: %.2f s\n", durationOfRehydration);
  Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
  Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
  Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
  Serial.printf("  Number of cycles: %d\n", numberOfCycles);
}

void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.println("WebSocket connected");
    // Send heartbeat packet
    {
      ArduinoJson::DynamicJsonDocument doc(64);
      doc["from"] = "esp32";
      doc["type"] = "heartbeat";
      char buffer[64];
      serializeJson(doc, buffer);
      webSocket.sendTXT(buffer);
      Serial.println("Sent heartbeat packet to frontend.");
    }
    break;
  case WStype_DISCONNECTED:
    Serial.println("WebSocket disconnected");
    break;
  case WStype_TEXT:
  {
    Serial.printf("Received: %s\n", payload);
    ArduinoJson::DynamicJsonDocument doc(300);
    auto err = deserializeJson(doc, payload, length);
    if (err)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(err.c_str());
      break;
    }

    if (doc["type"] == "espRecoveryState" && doc["data"].is<JsonObject>())
    {
      JsonObject data = doc["data"].as<JsonObject>();
      handleRecoveryPacket(data);
    }
    else if (doc["type"] == "parameters" && doc["data"].is<JsonObject>())
    {
      JsonObject data = doc["data"].as<JsonObject>();
      // ...existing parameter handling code...
    }
    else if (doc["name"].is<const char *>() && doc["state"].is<const char *>())
    {
      String name = doc["name"].as<String>();
      String state = doc["state"].as<String>();
      Serial0.printf("Parsed: name = %s, state = %s\n", name.c_str(), state.c_str());

      handleStateCommand(name, state);
    }
    else if (doc["type"] == "initialGpioState")
    {
      Serial.println("Initial GPIO state received, ignoring for now.");
    }
    else
    {
      Serial.println("Invalid packet received");
    }
    break;
  }
  default:
    break;
  }

  unsigned long lastSent = 0;
  void sendHeartbeat()
  {
    ArduinoJson::JsonDocument doc;
    doc["from"] = "esp32";
    doc["type"] = "heartbeat";
    char buffer[64];
    serializeJson(doc, buffer);
    webSocket.sendTXT(buffer);
    Serial.println("Sent heartbeat packet to frontend.");
  }

  // CYCLE PROGRESS COMMUNICATION

  void sendTemperature()
  {
    float temp = HEATING_Measure_Temp_Avg();
    ArduinoJson::DynamicJsonDocument doc(100);
    doc["type"] = "temperature";
    doc["value"] = temp;

    char buffer[100];
    serializeJson(doc, buffer);
    webSocket.sendTXT(buffer);
    Serial.printf("Sent temp: %.2f \u00b0C\n", temp);
  }

  // void sendSyringePercentage(float)
  // {
  //   float temp = HEATING_Measure_Temp_Avg();
  //   ArduinoJson::DynamicJsonDocument doc(100);
  //   doc["type"] = "temperature";
  //   doc["value"] = temp;

  //   char buffer[100];
  //   serializeJson(doc, buffer);
  //   webSocket.sendTXT(buffer);
  //   Serial.printf("Sent temp: %.2f \u00b0C\n", temp);
  // }

  // void sendCycleProgress(float)
  // {
  //   float temp = HEATING_Measure_Temp_Avg();
  //   ArduinoJson::DynamicJsonDocument doc(100);
  //   doc["type"] = "temperature";
  //   doc["value"] = temp;

  //   char buffer[100];
  //   serializeJson(doc, buffer);
  //   webSocket.sendTXT(buffer);
  //   Serial.printf("Sent temp: %.2f \u00b0C\n", temp);
  // }

  // void sendExtractReady(int)
  // {
  //   float temp = HEATING_Measure_Temp_Avg();
  //   ArduinoJson::DynamicJsonDocument doc(100);
  //   doc["type"] = "temperature";
  //   doc["value"] = temp;

  //   char buffer[100];
  //   serializeJson(doc, buffer);
  //   webSocket.sendTXT(buffer);
  //   Serial.printf("Sent temp: %.2f \u00b0C\n", temp);
  // }

  // void sendRecovery(int)
  // {
  //   float temp = HEATING_Measure_Temp_Avg();
  //   ArduinoJson::DynamicJsonDocument doc(100);
  //   doc["type"] = "temperature";
  //   doc["value"] = temp;

  //   char buffer[100];
  //   serializeJson(doc, buffer);
  //   webSocket.sendTXT(buffer);
  //   Serial.printf("Sent temp: %.2f \u00b0C\n", temp);
  // }

  // ERROR ENUM CODEs

  // void sendError(int)
  // {
  //   float temp = HEATING_Measure_Temp_Avg();
  //   ArduinoJson::DynamicJsonDocument doc(100);
  //   doc["type"] = "temperature";
  //   doc["value"] = temp;

  //   char buffer[100];
  //   serializeJson(doc, buffer);
  //   webSocket.sendTXT(buffer);
  //   Serial.printf("Sent temp: %.2f \u00b0C\n", temp);
  // }

#ifdef TESTING_MAIN

  void setup()
  {

    Serial.begin(115200);
    delay(2000); // Allow USB Serial to connect

    // Wi-Fi connect
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

    webSocket.begin(ServerIP, ServerPort, "/ws");
    webSocket.onEvent(onWebSocketEvent);

    Serial0.print("ESP32 MAC Address: ");
    Serial0.println(WiFi.macAddress());
    HEATING_Init();
    MIXING_Init();

    MOVEMENT_ConfigureInterrupts();
    REHYDRATION_ConfigureInterrupts();
  }

  void loop()
  {
    webSocket.loop();
    MOVEMENT_HandleInterrupts();
    REHYDRATION_HandleInterrupts();

    unsigned long now = millis();
    switch (currentState)
    {
    case SystemState::IDLE:
      if (now - lastSent >= 1000)
      {
        sendHeartbeat();
        lastSent = now;
      }
      break;
    case SystemState::READY:
      if (now - lastSent >= 1000)
      {
        sendHeartbeat();
        lastSent = now;
      }
      break;
    case SystemState::PAUSED:
      if (now - lastSent >= 1000)
      {
        sendHeartbeat();
        lastSent = now;
      }
      break;

    case SystemState::REHYDRATING:
      if (now - lastSent >= 1000)
      {
        sendTemperature();
        // sendSyringePercentage();
        // sendCycleProgress();
        lastSent = now;
      }
      break;

    case SystemState::HEATING:
      if (now - lastSent >= 1000)
      {
        sendTemperature();
        // sendCycleProgress();
        lastSent = now;
      }
      break;

    case SystemState::MIXING:
      if (now - lastSent >= 1000)
      {
        sendTemperature();
        // sendCycleProgress();
        lastSent = now;
      }
      break;

    case SystemState::REFILLING:
      Serial.println("Refilling...");
      currentState = SystemState::PAUSED;
      break;

    case SystemState::EXTRACTING:
      Serial.println("Extracting...");
      currentState = SystemState::PAUSED;
      break;

    case SystemState::LOGGING:
      Serial.println("Logging data...");
      currentState = previousState;
      break;

    case SystemState::ENDED:
      currentState = SystemState::IDLE;
      break;

    case SystemState::ERROR:
      Serial.println("System error — awaiting reset or external command.");
      break;
    }
    delay(10);
  }

#endif // TESTING_MAIN

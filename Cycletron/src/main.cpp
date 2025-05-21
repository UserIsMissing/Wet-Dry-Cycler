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
#define ServerIP "10.0.0.166"
#define ServerPort 5175

// === Wi-Fi Credentials ===
// const char* ssid = "UCSC-Devices";
// const char* password = "o9ANAjrZ9zkjYKy2yL";

const char *ssid = "DonnaHouse";
const char *password = "guessthepassword";

// const char *ssid = "TheDawgHouse";
// const char *password = "ThrowItBackForPalestine";

// const char *ssid = "UCSC-Guest";
// const char *password = "";

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

void sendCurrentState()
{
  const char *stateStr;

  switch (currentState)
  {
  case SystemState::IDLE:
    stateStr = "IDLE";
    break;
  case SystemState::READY:
    stateStr = "READY";
    break;
  case SystemState::REHYDRATING:
    stateStr = "REHYDRATING";
    break;
  case SystemState::HEATING:
    stateStr = "HEATING";
    break;
  case SystemState::MIXING:
    stateStr = "MIXING";
    break;
  case SystemState::REFILLING:
    stateStr = "REFILLING";
    break;
  case SystemState::EXTRACTING:
    stateStr = "EXTRACTING";
    break;
  case SystemState::LOGGING:
    stateStr = "LOGGING";
    break;
  case SystemState::PAUSED:
    stateStr = "PAUSED";
    break;
  case SystemState::ENDED:
    stateStr = "ENDED";
    break;
  case SystemState::ERROR:
    stateStr = "ERROR";
    break;
  default:
    stateStr = "UNKNOWN";
    break;
  }

  ArduinoJson::DynamicJsonDocument doc(100);
  doc["type"] = "currentState";
  doc["value"] = stateStr;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("Sent current state: %s\n", stateStr);
}

void setState(SystemState newState)
{
  if (currentState != SystemState::PAUSED &&
      currentState != SystemState::REFILLING &&
      currentState != SystemState::EXTRACTING)
  {
    previousState = currentState;
  }
  currentState = newState;
  sendCurrentState(); // Inform frontend of the state change
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
      JsonObject parameters = doc["data"].as<JsonObject>();

      volumeAddedPerCycle = atof(parameters["volumeAddedPerCycle"] | "0");
      durationOfRehydration = atof(parameters["durationOfRehydration"] | "0");
      syringeDiameter = atof(parameters["syringeDiameter"] | "0");
      desiredHeatingTemperature = atof(parameters["desiredHeatingTemperature"] | "0");
      durationOfHeating = atof(parameters["durationOfHeating"] | "0");
      durationOfMixing = atof(parameters["durationOfMixing"] | "0");
      numberOfCycles = atoi(parameters["numberOfCycles"] | "0");

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

      Serial.println("[PARAMETERS] Parameters received and parsed.");
      Serial.printf("  Volume per cycle: %.2f µL\n", volumeAddedPerCycle);
      Serial.printf("  Rehydration duration: %.2f s\n", durationOfRehydration);
      Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
      Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
      Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
      Serial.printf("  Number of cycles: %d\n", numberOfCycles);

      // Now transition into READY state

      setState(SystemState::READY);
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

int syringeStepCount = 0; // Cumulative steps pushed

void sendSyringePercentage()
{
  float percentUsed = (float)syringeStepCount / (float)MAX_SYRINGE_STEPS * 100.0;

  ArduinoJson::DynamicJsonDocument doc(100);
  doc["type"] = "syringePercentage";
  doc["value"] = percentUsed;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("Sent syringe percentage: %.2f%%\n", percentUsed);
}

unsigned long heatingStartTime = 0;
bool heatingStarted = false;

void sendHeatingProgress()
{
  unsigned long elapsed = millis() - heatingStartTime;
  float percentDone = ((float)elapsed / (durationOfHeating * 1000.0)) * 100.0;

  if (percentDone > 100.0)
    percentDone = 100.0; // Clamp to 100%

  ArduinoJson::DynamicJsonDocument doc(100);
  doc["type"] = "heatingProgress";
  doc["value"] = percentDone;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("Sent heating progress: %.2f%%\n", percentDone);
}

unsigned long mixingStartTime = 0;
bool mixingStarted = false;
void sendMixingProgress()
{
  unsigned long elapsed = millis() - mixingStartTime;
  float percentDone = ((float)elapsed / (durationOfMixing * 1000.0)) * 100.0;

  if (percentDone > 100.0)
    percentDone = 100.0;

  ArduinoJson::DynamicJsonDocument doc(100);
  doc["type"] = "mixingProgress";
  doc["value"] = percentDone;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("Sent mixing progress: %.2f%%\n", percentDone);
}

int completedCycles = 0; // How many full cycles have been completed
void sendCycleProgress()
{
  float percentDone = (numberOfCycles > 0)
                          ? ((float)completedCycles / (float)numberOfCycles) * 100.0
                          : 0.0;

  if (percentDone > 100.0)
    percentDone = 100.0;

  ArduinoJson::DynamicJsonDocument doc(100);
  doc["type"] = "cycleProgress";
  doc["completed"] = completedCycles;
  doc["total"] = numberOfCycles;
  doc["percent"] = percentDone;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("Sent cycle progress: %d/%d (%.2f%%)\n",
                completedCycles, numberOfCycles, percentDone);
}
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
  {
    sendCurrentState();
    Serial.println("[STATE] Rehydrating...");

    float uL_per_step = calculate_uL_per_step(syringeDiameter);
    int stepsToMove = (int)(volumeAddedPerCycle / uL_per_step);

    Serial.printf("[REHYDRATION] Dispensing %.2f uL of water using a %.2f inch diameter syringe (%d steps).\n",
                  volumeAddedPerCycle, syringeDiameter, stepsToMove);

    if (syringeStepCount + stepsToMove > MAX_SYRINGE_STEPS)
    {
      Serial.println("[ERROR] Syringe step count would exceed safe range! Aborting push.");
      currentState = SystemState::ERROR;
      break;
    }

    syringeStepCount += stepsToMove;
    Rehydration_Push((uint32_t)volumeAddedPerCycle, syringeDiameter);

    sendSyringePercentage();

    currentState = SystemState::MIXING;
    break;
  }

  case SystemState::MIXING:
  {
    if (!mixingStarted)
    {
      Serial.printf("[MIXING] Starting. Duration = %.2f seconds\n", durationOfMixing);
      mixingStartTime = millis();
      mixingStarted = true;

      // Turn on only specified zones (pins 11, 12, 13 assumed)
      for (int i = 0; i < sampleZoneCount; i++)
      {
        int zone = sampleZonesArray[i];
        int pin = -1;

        switch (zone)
        {
        case 1:
          pin = 11;
          break;
        case 2:
          pin = 12;
          break;
        case 3:
          pin = 13;
          break;
        default:
          Serial.printf("[MIXING] Invalid zone: %d\n", zone);
          continue;
        }

        Serial.printf("[MIXING] Turning ON motor for zone %d (GPIO %d)\n", zone, pin);
        MIXING_Motor_OnPin(pin);
      }
    }

    if (now - lastSent >= 1000)
    {
      sendMixingProgress();
      lastSent = now;
    }

    unsigned long elapsed = millis() - mixingStartTime;
    if (elapsed >= (unsigned long)(durationOfMixing * 1000))
    {
      Serial.println("[MIXING] Completed. Turning off motors.");
      MIXING_AllMotors_Off;
      mixingStarted = false;
      currentState = SystemState::HEATING; // or whatever your next state is
    }
    break;
  }
  

 

case SystemState::HEATING:
{
  if (!heatingStarted)
  {
    Serial.printf("[HEATING] Starting. Target = %.2f °C, Duration = %.2f s\n", desiredHeatingTemperature, durationOfHeating);
    heatingStartTime = millis();
    heatingStarted = true;
  }

  // Perform bang-bang control
  HEATING_Set_Temp((int)desiredHeatingTemperature);

  if (now - lastSent >= 1000)
  {
    sendTemperature();     // Send live temp
    sendHeatingProgress(); // Send % progress
    lastSent = now;
  }

  unsigned long elapsed = millis() - heatingStartTime;
  if (elapsed >= (unsigned long)(durationOfHeating * 1000))
  {
    Serial.println("[HEATING] Completed. Turning off heater and transitioning to MIXING.");
    HEATING_Off(); // You can add this helper or just: digitalWrite(HEATING_GPIO, LOW);
    completedCycles++;
    sendCycleProgress(); // Send cycle progress
    heatingStarted = false;
    currentState = SystemState::REHYDRATING;
  }
  break;
}

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

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "HEATING.h"
#include "MIXING.h"
#include "REHYDRATION.h"
#include "MOVEMENT.h"
#include "globals.h"
#include "send_functions.h"
#include "handle_functions.h" // <-- Add this include

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

// === Operational Parameters ===
// main.cpp
float volumeAddedPerCycle = 0;
float syringeDiameter = 0;
float desiredHeatingTemperature = 0;
float durationOfHeating = 0;
float durationOfMixing = 0;
int numberOfCycles = 0;
int syringeStepCount = 0;
unsigned long heatingStartTime = 0;
unsigned long mixingStartTime = 0;





unsigned long lastSent = 0; // Last time a message was sent to the server


// OTHER PARAMETERS NEEDED FOR RECOVERY
bool heatingStarted = false;
bool mixingStarted = false;
bool refillingStarted = false;
int completedCycles = 0;
int currentCycle = 0;
float heatingProgressPercent = 0;
float mixingProgressPercent = 0;


float heatingDurationRemaining = 0; // Remaining time for heating
float mixingDurationRemaining = 0;  // Remaining time for mixing

// Restore sample zones
// Or, use a fixed array if you prefer:
int sampleZonesArray[3];
int sampleZoneCount = 0;

SystemState currentState = SystemState::IDLE;
SystemState previousState = SystemState::IDLE;

// === WebSocket Client ===
WebSocketsClient webSocket;

unsigned long pausedElapsedTime = 0;
unsigned long pausedAtTime = 0;

void setState(SystemState newState)
{
  // --- PAUSE/RESUME LOGIC ---
  if (newState == SystemState::PAUSED ||
      newState == SystemState::EXTRACTING ||
      newState == SystemState::REFILLING) {
    pausedAtTime = millis();
    // Save elapsed time for the current state
    if (currentState == SystemState::HEATING) {
      pausedElapsedTime = millis() - heatingStartTime;
    } else if (currentState == SystemState::MIXING) {
      pausedElapsedTime = millis() - mixingStartTime;
    } else {
      pausedElapsedTime = 0;
    }
  }
  // If resuming from PAUSED, EXTRACTING, or REFILLING, adjust start times and remaining durations
  if ((currentState == SystemState::PAUSED ||
       currentState == SystemState::EXTRACTING ||
       currentState == SystemState::REFILLING) &&
      (newState != SystemState::PAUSED &&
       newState != SystemState::EXTRACTING &&
       newState != SystemState::REFILLING)) {
    if (previousState == SystemState::HEATING) {
      heatingStartTime = millis() - pausedElapsedTime;
      heatingDurationRemaining -= pausedElapsedTime;
      if (heatingDurationRemaining < 0) heatingDurationRemaining = 0;
    } else if (previousState == SystemState::MIXING) {
      mixingStartTime = millis() - pausedElapsedTime;
      mixingDurationRemaining -= pausedElapsedTime;
      if (mixingDurationRemaining < 0) mixingDurationRemaining = 0;
    }
    pausedElapsedTime = 0;
    pausedAtTime = 0;
  }
  // --- END PAUSE/RESUME LOGIC ---

  if (currentState != SystemState::PAUSED &&
      currentState != SystemState::REFILLING &&
      currentState != SystemState::EXTRACTING)
  {
    previousState = currentState;
  }
  currentState = newState;
  sendCurrentState(); // Inform frontend of the state change
}

void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.println("WebSocket connected");
    // Send heartbeat packet
    {
      ArduinoJson::JsonDocument doc; // auto-resizing with latest versions
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
    ArduinoJson::JsonDocument doc; // auto-resizing with latest versions
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
      JsonObject parameters = doc["data"].as<JsonObject>();
      handleParametersPacket(parameters);
    }
    else if (doc["name"].is<const char *>() && doc["state"].is<const char *>())
    {
      String name = doc["name"].as<String>();
      String state = doc["state"].as<String>();
      Serial0.printf("Parsed: name = %s, state = %s\n", name.c_str(), state.c_str());

      handleStateCommand(name, state);
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
    if (currentCycle >= numberOfCycles)
    {
      Serial.println("[REHYDRATION] Final cycle already completed. Sending end packet and switching to ENDED.");
      sendEndOfCycles();
      currentState = SystemState::ENDED;
      sendCurrentState(); // Notify state change to ENDED
      break;
    }
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
    sendCurrentState(); // Notify state change to MIXING
    break;
  }

  case SystemState::MIXING:
  {
    if (!mixingStarted)
    {
      Serial.println("[MIXING] Starting...");

      // Decide how long to mix based on whether we're recovering
      unsigned long mixTime = heatingStarted ? mixingDurationRemaining : (durationOfMixing * 1000);
      mixingStartTime = millis();
      mixingDurationRemaining = mixTime;
      mixingStarted = true;

      // Turn on motors for the selected sample zones
      for (int i = 0; i < sampleZoneCount; i++)
      {
        int zone = sampleZonesArray[i];
        int pin = (zone == 1) ? 11 : (zone == 2) ? 12
                                 : (zone == 3)   ? 13
                                                 : -1;
        if (pin != -1)
        {
          Serial.printf("[MIXING] Motor ON for zone %d (GPIO %d)\n", zone, pin);
          MIXING_Motor_OnPin(pin);
        }
      }
    }

    // Send progress update every second
    if (now - lastSent >= 1000)
    {
      sendMixingProgress();
      lastSent = now;
    }

    // Check if the mixing duration has passed
    if (millis() - mixingStartTime >= mixingDurationRemaining)
    {
      Serial.println("[MIXING] Done. Turning off motors.");
      MIXING_AllMotors_Off;
      mixingStarted = false;
      currentState = SystemState::HEATING;
      sendCurrentState(); // Notify state change to HEATING
    }
    break;
  }

  case SystemState::HEATING:
  {
    if (!heatingStarted)
    {
      Serial.println("[HEATING] Starting...");
      unsigned long heatTime = heatingProgressPercent > 0
                                   ? (unsigned long)((1.0 - (heatingProgressPercent / 100.0)) * durationOfHeating * 1000)
                                   : (unsigned long)(durationOfHeating * 1000);

      // If resuming from pause, use heatingDurationRemaining if set
      if (heatingDurationRemaining > 0 && heatingDurationRemaining < heatTime) {
        heatTime = heatingDurationRemaining;
      }

      heatingStartTime = millis();
      heatingDurationRemaining = heatTime;
      heatingStarted = true;
    }

    // Control the heater
    HEATING_Set_Temp((int)desiredHeatingTemperature);

    // Send telemetry every second
    if (now - lastSent >= 1000)
    {
      sendTemperature();

      // Send progress
      unsigned long elapsed = millis() - heatingStartTime;
      float percentDone = ((float)elapsed / heatingDurationRemaining) * 100.0;
      if (percentDone > 100.0)
        percentDone = 100.0;

      ArduinoJson::JsonDocument doc; // auto-resizing with latest versions
     sendHeatingProgress();

      lastSent = now;
    }

    // Check if heating is complete
    if (millis() - heatingStartTime >= heatingDurationRemaining)
    {
      Serial.println("[HEATING] Done. Turning off heater.");
      HEATING_Off();
      heatingStarted = false;
      completedCycles++;
      currentCycle++;
      sendCycleProgress();
      currentState = SystemState::REHYDRATING;
      sendCurrentState(); // Notify state change to REHYDRATING
    }

    break;
  }

  case SystemState::REFILLING:
    if (!refillingStarted)
    {
      Serial.println("[STATE] REFILLING: Moving back until back bumper is hit");
      Rehydration_BackUntilBumper(); // Retract fully
      syringeStepCount = 0;          // Reset step counter
      sendSyringeResetInfo();        // Notify webserver
      refillingStarted = true;
      sendCurrentState(); // Notify state change to REFILLING
    }
    break;

  case SystemState::EXTRACTING:
    Serial.println("Extracting...");
    currentState = SystemState::PAUSED;
    sendCurrentState(); // Notify state change to PAUSED
    break;

  case SystemState::LOGGING:
    Serial.println("Logging data...");
    currentState = previousState;
    sendCurrentState(); // Notify state change to previousState
    break;

  case SystemState::ENDED:
    completedCycles = 0;
    currentCycle = 0;
    currentState = SystemState::IDLE;
    sendCurrentState(); // Notify state change to IDLE
    break;

  case SystemState::ERROR:
    Serial.println("System error — awaiting reset or external command.");
    sendCurrentState(); // Notify state change to ERROR
    break;
  }
  delay(10);
}

#endif // TESTING_MAIN

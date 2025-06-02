#include <ArduinoJson.h>
#include "globals.h"
#include "send_functions.h"
#include "handle_functions.h"

// Use globals from globals.h
extern float volumeAddedPerCycle;
extern float syringeDiameter;
extern float desiredHeatingTemperature;
extern float durationOfHeating;
extern float durationOfMixing;
extern int numberOfCycles;
extern int syringeStepCount;
extern unsigned long heatingStartTime;
extern unsigned long mixingStartTime;
extern bool heatingStarted;
extern bool mixingStarted;
extern int completedCycles;
extern int currentCycle;
extern float heatingProgressPercent;
extern float mixingProgressPercent;
extern int sampleZonesArray[3];
extern int sampleZoneCount;
extern SystemState currentState;
extern SystemState previousState;
extern void setState(SystemState newState);

void handleStateCommand(const String &name, const String &state)
{
  Serial.printf("[DEBUG] handleStateCommand: name=%s, state=%s\n", name.c_str(), state.c_str());
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
      setState(SystemState::PAUSED);
      Serial.println("State changed to PAUSED");
    }
    else
    {
      setState(previousState);
      Serial.printf("State RESUMED (currentState = %d)", static_cast<int>(currentState));
    }
  }
  else if (name == "endCycle" && state == "on")
  {
    setState(SystemState::ENDED);
    Serial.println("State changed to ENDED");
  }
  else if (name == "extract")
  {
    if (state == "on")
    {
      setState(SystemState::EXTRACTING);
      Serial.println("Extraction started");
    }
    else
    {
      setState(previousState);
      Serial.println("Extraction ended — resuming");
    }
  }
  else if (name == "refill")
  {
    if (state == "on")
    {
      setState(SystemState::REFILLING);
      Serial.println("Refill started");
    }
    else
    {
      setState(previousState);
      Serial.println("Refill ended — resuming");
    }
  }
  else if (name == "logCycle" && state == "on")
  {

    setState(SystemState::LOGGING);
    Serial.println("State changed to LOGGING");
  }
  else
  {
    Serial.printf("Unknown or ignored command: %s with state: %s\n", name.c_str(), state.c_str());
  }
  // Only set recoveryStateDirty, never call sendEspRecoveryState here
  recoveryStateDirty = true;
  Serial.println("[DEBUG] handleStateCommand: Marked recoveryStateDirty = true");
}

void handleRecoveryPacket(const JsonObject &data)
{
  Serial.println("[DEBUG] handleRecoveryPacket called");
  
  // Check if we have valid recovery data
  if (data.isNull() || data.size() == 0) {
    Serial.println("Recovery packet is empty or invalid. Transitioning to IDLE state.");
    setState(SystemState::IDLE);
    return;
  }

  // Restore the parameters first
  volumeAddedPerCycle = data["volumeAddedPerCycle"].is<float>() ? data["volumeAddedPerCycle"].as<float>() : 0.0;
  syringeDiameter = data["syringeDiameter"].is<float>() ? data["syringeDiameter"].as<float>() : 0.0;
  desiredHeatingTemperature = data["desiredHeatingTemperature"].is<float>() ? data["desiredHeatingTemperature"].as<float>() : 0.0;
  durationOfHeating = data["durationOfHeating"].is<float>() ? data["durationOfHeating"].as<float>() : 0.0;
  durationOfMixing = data["durationOfMixing"].is<float>() ? data["durationOfMixing"].as<float>() : 0.0;
  numberOfCycles = data["numberOfCycles"].is<int>() ? data["numberOfCycles"].as<int>() : 0;
  syringeStepCount = data["syringeStepCount"].is<int>() ? data["syringeStepCount"].as<int>() : 0;
  heatingStartTime = data["heatingStartTime"].is<unsigned long>() ? data["heatingStartTime"].as<unsigned long>() : 0;
  heatingStarted = data["heatingStarted"].is<bool>() ? data["heatingStarted"].as<bool>() : false;
  mixingStartTime = data["mixingStartTime"].is<unsigned long>() ? data["mixingStartTime"].as<unsigned long>() : 0;
  mixingStarted = data["mixingStarted"].is<bool>() ? data["mixingStarted"].as<bool>() : false;
  completedCycles = data["completedCycles"].is<int>() ? data["completedCycles"].as<int>() : 0;
  currentCycle = data["currentCycle"].is<int>() ? data["currentCycle"].as<int>() : 0;
  heatingProgressPercent = data["heatingProgress"].is<float>() ? data["heatingProgress"].as<float>() : 0.0;
  mixingProgressPercent = data["mixingProgress"].is<float>() ? data["mixingProgress"].as<float>() : 0.0;

  // Restore sample zones
  sampleZoneCount = 0;
  if (data["sampleZonesToMix"].is<JsonArray>())
  {
    JsonArray zones = data["sampleZonesToMix"].as<JsonArray>();
    for (JsonVariant val : zones)
    {
      if (val.is<int>() && sampleZoneCount < 3)
      {
        sampleZonesArray[sampleZoneCount++] = val.as<int>();
      }
    }
  }

  // Restore the state last
  int stateInt = data["currentState"].is<int>() ? data["currentState"].as<int>() : (int)SystemState::IDLE;
  SystemState recoveredState = (SystemState)stateInt;
  
  // Adjust timing for states that are time-dependent
  unsigned long currentTime = millis();
  if (recoveredState == SystemState::HEATING && heatingStarted) {
    // Adjust heating start time based on progress
    float progressFraction = heatingProgressPercent / 100.0;
    unsigned long elapsedTime = (unsigned long)(progressFraction * durationOfHeating * 1000.0);
    heatingStartTime = currentTime - elapsedTime;
  }
  
  if (recoveredState == SystemState::MIXING && mixingStarted) {
    // Adjust mixing start time based on progress
    float progressFraction = mixingProgressPercent / 100.0;
    unsigned long elapsedTime = (unsigned long)(progressFraction * durationOfMixing * 1000.0);
    mixingStartTime = currentTime - elapsedTime;
  }

  setState(recoveredState);

  // Mark recovery as clean since we just restored from it
  recoveryStateDirty = false;

  // Log recovery details
  Serial.println("[RECOVERY] Restored system state and parameters:");
  Serial.printf("  Current state: %d\n", (int)recoveredState);
  Serial.printf("  Volume per cycle: %.2f µL\n", volumeAddedPerCycle);
  Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
  Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
  Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
  Serial.printf("  Number of cycles: %d (completed: %d, current: %d)\n", numberOfCycles, completedCycles, currentCycle);
  Serial.printf("  Syringe Step Count: %d\n", syringeStepCount);
  Serial.printf("  HeatingStarted: %s | HeatingProgress: %.2f%%\n", heatingStarted ? "true" : "false", heatingProgressPercent);
  Serial.printf("  MixingStarted: %s | MixingProgress: %.2f%%\n", mixingStarted ? "true" : "false", mixingProgressPercent);
}

void handleParametersPacket(const JsonObject &parameters)
{
  volumeAddedPerCycle = parameters["volumeAddedPerCycle"].is<const char *>() ? atof(parameters["volumeAddedPerCycle"].as<const char *>()) : 0.0;
  syringeDiameter = parameters["syringeDiameter"].is<const char *>() ? atof(parameters["syringeDiameter"].as<const char *>()) : 0.0;
  desiredHeatingTemperature = parameters["desiredHeatingTemperature"].is<const char *>() ? atof(parameters["desiredHeatingTemperature"].as<const char *>()) : 0.0;
  durationOfHeating = parameters["durationOfHeating"].is<const char *>() ? atof(parameters["durationOfHeating"].as<const char *>()) : 0.0;
  durationOfMixing = parameters["durationOfMixing"].is<const char *>() ? atof(parameters["durationOfMixing"].as<const char *>()) : 0.0;
  numberOfCycles = parameters["numberOfCycles"].is<const char *>() ? atoi(parameters["numberOfCycles"].as<const char *>()) : 0;

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
  Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
  Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
  Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
  Serial.printf("  Number of cycles: %d\n", numberOfCycles);

  setState(SystemState::READY);
}

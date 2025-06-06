#include <ArduinoJson.h>
#include "globals.h"
#include "send_functions.h"
#include "handle_functions.h"
#include "globals.h"

void handleStateCommand(const String &name, const String &state)
{
  if (name != "vialSetup" && currentState == SystemState::IDLE)
  {
    Serial.println("System is IDLE — cannot process commands until Vial_Setup (yes/no) is received.");
    return;
  }

  if (name == "startCycle" && state == "on")
  {
    setState(SystemState::REHYDRATING);
    Serial.println("State changed to REHYDRATING");
    return;
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
    return;
  }
  else if (name == "vialSetup")
  {
    if (state == "yes")
    {
      setState(SystemState::VIAL_SETUP);
      shouldMoveForward = true; // Enable back-and-forth movement
      Serial.println("State changed to VIAL_SETUP");
    }
    else if (state == "continue")
    {
      shouldMoveBack = true; // Disable back-and-forth movement
    }
    else if (state == "no")
    {
      setState(SystemState::WAITING);
    }
    else
    {
      Serial.printf("[ERROR] Unknown state for vialSetup: %s\n", state.c_str());
      return; // Ignore unknown states
    }
    return;
  }
  else if (name == "endCycle" && state == "on")
  {
    setState(SystemState::ENDED);
    Serial.println("State changed to ENDED");
    return;
  }
  else if (name == "extract")
  {
    if (state == "on")
    {
      setState(SystemState::EXTRACTING);
      shouldMoveForward = true; // Enable back-and-forth movement
      Serial.println("Extraction started");
    }
    else
    {
      shouldMoveBack = true; // Disable back-and-forth movement
    }
    return;
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
    return;
  }
  else if (name == "logCycle" && state == "on")
  {
    setState(SystemState::LOGGING);
    Serial.println("State changed to LOGGING");
    return;
  }
  else if (name == "restartESP32" && state == "on")
  {
    Serial.println("Restart command received - restarting ESP32...");
    delay(100); // Brief delay to allow serial output
    ESP.restart();
    return;
  }
  else
  {
    Serial.printf("Unknown or ignored command: %s with state: %s\n", name.c_str(), state.c_str());
  }
}

void handleRecoveryPacket(const JsonObject &data)
{
  if (!data["currentState"].is<const char *>() || !data["parameters"].is<JsonObject>())
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
  // Restore parameters - handle both string and numeric values
  JsonObject parameters = data["parameters"];
  volumeAddedPerCycle = parameters["volumeAddedPerCycle"].is<const char *>() ? 
    atof(parameters["volumeAddedPerCycle"].as<const char *>()) : 
    parameters["volumeAddedPerCycle"].as<float>();
    
  syringeDiameter = parameters["syringeDiameter"].is<const char *>() ? 
    atof(parameters["syringeDiameter"].as<const char *>()) : 
    parameters["syringeDiameter"].as<float>();
    
  desiredHeatingTemperature = parameters["desiredHeatingTemperature"].is<const char *>() ? 
    atof(parameters["desiredHeatingTemperature"].as<const char *>()) : 
    parameters["desiredHeatingTemperature"].as<float>();
    
  durationOfHeating = parameters["durationOfHeating"].is<const char *>() ? 
    atof(parameters["durationOfHeating"].as<const char *>()) : 
    parameters["durationOfHeating"].as<float>();
    
  durationOfMixing = parameters["durationOfMixing"].is<const char *>() ? 
    atof(parameters["durationOfMixing"].as<const char *>()) : 
    parameters["durationOfMixing"].as<float>();
    
  numberOfCycles = parameters["numberOfCycles"].is<const char *>() ? 
    atoi(parameters["numberOfCycles"].as<const char *>()) : 
    parameters["numberOfCycles"].as<int>();  syringeStepCount = parameters["syringeStepCount"].is<const char *>() ? 
    atoi(parameters["syringeStepCount"].as<const char *>()) : 
    (parameters["syringeStepCount"].is<int>() ? parameters["syringeStepCount"].as<int>() : 0);
    
  heatingStartTime = parameters["heatingStartTime"].is<const char *>() ? 
    atol(parameters["heatingStartTime"].as<const char *>()) : 
    (parameters["heatingStartTime"].is<long>() ? parameters["heatingStartTime"].as<long>() : 0);
    
  heatingStarted = parameters["heatingStarted"].is<bool>() ? parameters["heatingStarted"].as<bool>() : false;
  
  mixingStartTime = parameters["mixingStartTime"].is<const char *>() ? 
    atol(parameters["mixingStartTime"].as<const char *>()) : 
    (parameters["mixingStartTime"].is<long>() ? parameters["mixingStartTime"].as<long>() : 0);
    
  mixingStarted = parameters["mixingStarted"].is<bool>() ? parameters["mixingStarted"].as<bool>() : false;
  
  completedCycles = parameters["completedCycles"].is<const char *>() ? 
    atoi(parameters["completedCycles"].as<const char *>()) : 
    (parameters["completedCycles"].is<int>() ? parameters["completedCycles"].as<int>() : 0);
    
  currentCycle = parameters["currentCycle"].is<const char *>() ? 
    atoi(parameters["currentCycle"].as<const char *>()) : 
    (parameters["currentCycle"].is<int>() ? parameters["currentCycle"].as<int>() : 0);
    
  heatingProgressPercent = parameters["heatingProgress"].is<const char *>() ? 
    atof(parameters["heatingProgress"].as<const char *>()) : 
    (parameters["heatingProgress"].is<float>() ? parameters["heatingProgress"].as<float>() : 0.0);
    
  mixingProgressPercent = parameters["mixingProgress"].is<const char *>() ? 
    atof(parameters["mixingProgress"].as<const char *>()) : 
    (parameters["mixingProgress"].is<float>() ? parameters["mixingProgress"].as<float>() : 0.0);
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

  if (heatingStarted == true)
  {
    float heatingDurationRemaining = (1.0 - (heatingProgressPercent / 100.0)) * durationOfHeating * 1000.0;
  }

  if (mixingStarted == true)
  {
    float mixingDurationRemaining = (1.0 - (mixingProgressPercent / 100.0)) * durationOfMixing * 1000.0;
  }

  // Log recovery details
  Serial.println("[RECOVERY] Restored system state and parameters:");
  Serial.printf("  Current state: %s\n", recoveredState.c_str());
  Serial.printf("  Volume per cycle: %.2f µL\n", volumeAddedPerCycle);
  Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
  Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
  Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
  Serial.printf("  Number of cycles: %d (completed: %d, current: %d)\n", numberOfCycles, completedCycles, currentCycle);
  Serial.printf("  Syringe Step Count: %d\n", syringeStepCount);
  Serial.printf("  HeatingStarted: %s | HeatingStartTime: %lu\n", heatingStarted ? "true" : "false", heatingProgressPercent);
  Serial.printf("  MixingStarted: %s | MixingStartTime: %lu\n", mixingStarted ? "true" : "false", mixingProgressPercent);
}

void handleParametersPacket(const JsonObject &parameters)
{
  // Handle both string and numeric values for parameters
  volumeAddedPerCycle = parameters["volumeAddedPerCycle"].is<const char *>() ? 
    atof(parameters["volumeAddedPerCycle"].as<const char *>()) : 
    parameters["volumeAddedPerCycle"].as<float>();
    
  syringeDiameter = parameters["syringeDiameter"].is<const char *>() ? 
    atof(parameters["syringeDiameter"].as<const char *>()) : 
    parameters["syringeDiameter"].as<float>();
    
  desiredHeatingTemperature = parameters["desiredHeatingTemperature"].is<const char *>() ? 
    atof(parameters["desiredHeatingTemperature"].as<const char *>()) : 
    parameters["desiredHeatingTemperature"].as<float>();
    
  durationOfHeating = parameters["durationOfHeating"].is<const char *>() ? 
    atof(parameters["durationOfHeating"].as<const char *>()) : 
    parameters["durationOfHeating"].as<float>();
    
  durationOfMixing = parameters["durationOfMixing"].is<const char *>() ? 
    atof(parameters["durationOfMixing"].as<const char *>()) : 
    parameters["durationOfMixing"].as<float>();
    
  numberOfCycles = parameters["numberOfCycles"].is<const char *>() ? 
    atoi(parameters["numberOfCycles"].as<const char *>()) : 
    parameters["numberOfCycles"].as<int>();

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

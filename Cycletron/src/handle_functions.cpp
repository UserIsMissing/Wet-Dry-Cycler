#include <ArduinoJson.h>
#include "globals.h"
#include "send_functions.h"
#include "handle_functions.h"
#include "globals.h"

/**
 * @brief Converts a command string to its corresponding CommandType enum.
 *
 * Supports known command keywords such as "vialSetup", "startCycle", etc.
 * Returns CommandType::UNKNOWN for unrecognized commands.
 *
 * @param name Command string from user or client
 * @return Corresponding CommandType enum
 */
CommandType parseCommand(const String &name)
{
    if (name == "vialSetup") return CommandType::VIAL_SETUP;
    if (name == "startCycle") return CommandType::START_CYCLE;
    if (name == "pauseCycle") return CommandType::PAUSE_CYCLE;
    if (name == "endCycle") return CommandType::END_CYCLE;
    if (name == "extract") return CommandType::EXTRACT;
    if (name == "refill") return CommandType::REFILL;
    if (name == "logCycle") return CommandType::LOG_CYCLE;
    if (name == "restartESP32") return CommandType::RESTART_ESP32;
    return CommandType::UNKNOWN;
}

/**
 * @brief Handles command and state transitions received from client.
 *
 * Interprets commands such as vial setup, start/pause/end cycle,
 * extraction, refill, and logging. Updates internal system state
 * and triggers mechanical actions accordingly.
 *
 * @param name Name of the command (e.g., "startCycle")
 * @param state Desired state or instruction (e.g., "on", "yes")
 */
void handleStateCommand(const String &name, const String &state)
{
    // Prevent commands unless system is out of IDLE (except for vialSetup)
    if (parseCommand(name) != CommandType::VIAL_SETUP && currentState == SystemState::IDLE)
    {
        Serial.println("[IGNORED] System is IDLE — waiting for vialSetup command.");
        return;
    }

    CommandType cmd = parseCommand(name);

    switch (cmd)
    {
    case CommandType::VIAL_SETUP:
        // Handle different states during vial setup
        if (state == "yes") {
            setState(SystemState::VIAL_SETUP);
            shouldMoveForward = true;
            Serial.println("State changed to VIAL_SETUP");
        } else if (state == "continue") {
            shouldMoveBack = true;
            Serial.println("Continuing vial setup (backward movement)");
        } else if (state == "no") {
            setState(SystemState::WAITING);
            Serial.println("State changed to WAITING");
        } else {
            Serial.printf("[ERROR] Unknown state for vialSetup: '%s'\n", state.c_str());
        }
        break;

    case CommandType::START_CYCLE:
        if (state == "on") {
            setState(SystemState::REHYDRATING);
            Serial.println("State changed to REHYDRATING");
        }
        break;

    case CommandType::PAUSE_CYCLE:
        if (state == "on") {
            setState(SystemState::PAUSED);
            Serial.println("State changed to PAUSED");
        } else {
            setState(previousState);
            Serial.printf("Resumed — currentState = %d\n", static_cast<int>(currentState));
        }
        break;

    case CommandType::END_CYCLE:
        if (state == "on") {
            setState(SystemState::ENDED);
            Serial.println("State changed to ENDED");
        }
        break;

    case CommandType::EXTRACT:
        if (state == "on") {
            setState(SystemState::EXTRACTING);
            shouldMoveForward = true;
            Serial.println("Extraction started");
        } else {
            shouldMoveBack = true;
            Serial.println("Extraction back movement requested");
        }
        break;

    case CommandType::REFILL:
        if (state == "on") {
            setState(SystemState::REFILLING);
            Serial.println("Refill started");
        } else if (state == "off") {
            refillingStarted = false;  // Reset the flag for next time
            setState(previousState);
            Serial.println("Refill ended — resuming previous state");
        }
        break;

    case CommandType::LOG_CYCLE:
        if (state == "on") {
            setState(SystemState::LOGGING);
            Serial.println("State changed to LOGGING");
        }
        break;

    case CommandType::RESTART_ESP32:
        if (state == "on") {
            Serial.println("Restart command received — restarting ESP32...");
            delay(100);
            ESP.restart();
        }
        break;

    case CommandType::UNKNOWN:
    default:
        Serial.printf("[ERROR] Unknown or unhandled command: name = '%s', state = '%s'\n", name.c_str(), state.c_str());
        break;
    }
}

/**
 * @brief Restores system state and parameters from a JSON recovery packet.
 *
 * Used to recover previous state and operational parameters after restart or crash.
 * Parses the state string and various configuration values.
 *
 * @param data JSON object containing recovery state and parameters
 */
void handleRecoveryPacket(const JsonObject &data)
{
  if (!data["currentState"].is<const char *>() || !data["parameters"].is<JsonObject>()) {
    Serial.println("Recovery packet is empty or invalid. Transitioning to IDLE state.");
    currentState = SystemState::IDLE;
    return;
  }

  // Restore the last known operational state
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
  volumeAddedPerCycle = parameters["volumeAddedPerCycle"].is<float>() ? parameters["volumeAddedPerCycle"].as<float>() : 0.0;
  syringeDiameter = parameters["syringeDiameter"].is<float>() ? parameters["syringeDiameter"].as<float>() : 0.0;
  desiredHeatingTemperature = parameters["desiredHeatingTemperature"].is<int>() ? parameters["desiredHeatingTemperature"].as<int>() : 0.0;
  durationOfHeating = parameters["durationOfHeating"].is<float>() ? parameters["durationOfHeating"].as<float>() : 0.0;
  durationOfMixing = parameters["durationOfMixing"].is<float>() ? parameters["durationOfMixing"].as<float>() : 0.0;
  numberOfCycles = parameters["numberOfCycles"].is<int>() ? parameters["numberOfCycles"].as<int>() : 0;
  syringeStepCount = parameters["syringeStepCount"].is<int>() ? parameters["syringeStepCount"].as<int>() : 0;
  heatingStartTime = parameters["heatingStartTime"].is<long>() ? parameters["heatingStartTime"].as<long>() : 0;
  heatingStarted = parameters["heatingStarted"].is<bool>() ? parameters["heatingStarted"].as<bool>() : false;
  
  mixingStartTime = parameters["mixingStartTime"].is<const char *>() ? 
    atol(parameters["mixingStartTime"].as<const char *>()) : 
    (parameters["mixingStartTime"].is<long>() ? parameters["mixingStartTime"].as<long>() : 0);
    
  mixingStarted = parameters["mixingStarted"].is<bool>() ? parameters["mixingStarted"].as<bool>() : false;
  completedCycles = parameters["completedCycles"].is<int>() ? parameters["completedCycles"].as<int>() : 0;
  currentCycle = parameters["currentCycle"].is<int>() ? parameters["currentCycle"].as<int>() : 0;
  heatingProgressPercent = parameters["heatingProgress"].is<float>() ? parameters["heatingProgress"].as<float>() : 0.0;
  mixingProgressPercent = parameters["mixingProgress"].is<float>() ? parameters["mixingProgress"].as<float>() : 0.0;
  // Restore sample zones
  sampleZoneCount = 0;
  if (parameters["sampleZonesToMix"].is<JsonArray>()) {
    JsonArray zones = parameters["sampleZonesToMix"].as<JsonArray>();
    for (JsonVariant val : zones) {
      if (val.is<int>() && sampleZoneCount < 10) {
        sampleZonesArray[sampleZoneCount++] = val.as<int>();
      }
    }
  }

  // Print recovery state for debugging
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

/**
 * @brief Parses and applies configuration parameters from client.
 *
 * Receives parameters like syringe diameter, heating time, and sample zones.
 * Updates global configuration variables accordingly.
 *
 * @param parameters JSON object containing the configuration parameters
 */
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

  // Restore mixing zones
  sampleZoneCount = 0;
  if (parameters["sampleZonesToMix"].is<JsonArray>()) {
    JsonArray zones = parameters["sampleZonesToMix"].as<JsonArray>();
    for (JsonVariant val : zones) {
      if (val.is<int>() && sampleZoneCount < 10) {
        sampleZonesArray[sampleZoneCount++] = val.as<int>();
      }
    }
  }

  // Print configuration summary
  Serial.println("[PARAMETERS] Parameters received and parsed.");
  Serial.printf("  Volume per cycle: %.2f µL\n", volumeAddedPerCycle);
  Serial.printf("  Syringe diameter: %.2f in\n", syringeDiameter);
  Serial.printf("  Heating temp: %.2f °C for %.2f s\n", desiredHeatingTemperature, durationOfHeating);
  Serial.printf("  Mixing duration: %.2f s with %d zone(s)\n", durationOfMixing, sampleZoneCount);
  Serial.printf("  Number of cycles: %d\n", numberOfCycles);

  // Ready the system for operation
  setState(SystemState::READY);
}



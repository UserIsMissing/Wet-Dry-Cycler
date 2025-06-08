#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "HEATING.h"
#include "globals.h"
#include "send_functions.h"
#include "REHYDRATION.h"


void sendHeartbeat()
{
  ArduinoJson::JsonDocument doc;
  doc["type"] = "heartbeat";
  doc["value"] = 1;
  char buffer[64];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[%d] Sent heartbeat packet to frontend.\n", static_cast<int>(currentState));
}

void sendTemperature()
{
  float temp = HEATING_Measure_Temp_Avg();
  ArduinoJson::JsonDocument doc;
  doc["type"] = "temperature";
  doc["value"] = temp;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent temp: %.2f \u00b0C\n", temp);
}

void sendSyringePercentage()
{
  float percentUsed = ((float)syringeStepCount / (float)MAX_SYRINGE_STEPS * 100.0);

  ArduinoJson::JsonDocument doc;
  doc["type"] = "syringePercentage";
  doc["value"] = percentUsed;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent syringe percentage remaining: %.2f%%\n", percentUsed);
}

void sendHeatingProgress()
{
  unsigned long elapsed = millis() - heatingStartTime;
  float percentDone = ((float)elapsed / (durationOfHeating * 1000.0)) * 100.0;

  if (percentDone > 100.0)
    percentDone = 100.0;

  ArduinoJson::JsonDocument doc;
  doc["type"] = "heatingProgress";
  doc["value"] = percentDone;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent heating progress: %.2f%%\n", percentDone);
}

void sendMixingProgress()
{
  unsigned long elapsed = millis() - mixingStartTime;
  float percentDone = ((float)elapsed / (durationOfMixing * 1000.0)) * 100.0;

  if (percentDone > 100.0)
    percentDone = 100.0;

  ArduinoJson::JsonDocument doc;
  doc["type"] = "mixingProgress";
  doc["value"] = percentDone;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent mixing progress: %.2f%%\n", percentDone);
}

void sendCycleProgress()
{
  float percentDone = (numberOfCycles > 0)
                          ? ((float)completedCycles / (float)numberOfCycles) * 100.0
                          : 0.0;

  if (percentDone > 100.0)
    percentDone = 100.0;

  ArduinoJson::JsonDocument doc;
  doc["type"] = "cycleProgress";
  doc["completed"] = completedCycles;
  doc["total"] = numberOfCycles;
  doc["percent"] = percentDone;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent cycle progress: %d/%d (%.2f%%)\n",
                completedCycles, numberOfCycles, percentDone);
}

void sendEndOfCycles()
{
  ArduinoJson::JsonDocument doc;
  doc["type"] = "endOfCycles";
  doc["message"] = "All cycles completed.";

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.println("[WS] Sent end of cycles packet to frontend.");
}

void sendSyringeResetInfo()
{
  ArduinoJson::JsonDocument doc;
  doc["type"] = "syringeReset";
  doc["steps"] = syringeStepCount;

  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);

  Serial.println("[WS] Sent syringe reset info");
}

void sendExtractionReady() 
{
    ArduinoJson::JsonDocument doc;
    doc["type"] = "status";
    doc["extractionReady"] = "ready";

    char buffer[100];
    serializeJson(doc, buffer);
    webSocket.sendTXT(buffer);
    Serial.println("[WS] Sent extraction ready notification");
}

void sendCurrentState()
{
  const char *stateStr;
  switch (currentState)
  {
  case SystemState::VIAL_SETUP:
    stateStr = "VIAL_SETUP";
    break;
  case SystemState::IDLE:
    stateStr = "IDLE";
    break;
  case SystemState::WAITING:
    stateStr = "WAITING";
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

  ArduinoJson::JsonDocument doc;
  doc["type"] = "currentState";
  doc["value"] = stateStr;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent current state: %s\n", stateStr);
}

// Add this function to send a recovery packet to the server
void sendRecoveryPacketToServer()
{
  ArduinoJson::JsonDocument doc;
  doc["type"] = "espRecoveryState";
  JsonObject data = doc["data"].to<JsonObject>();
  // Save current state and all relevant parameters
  switch (currentState)
  {
  case SystemState::IDLE:
    data["currentState"] = "IDLE";
    break;
  case SystemState::READY:
    data["currentState"] = "READY";
    break;
  case SystemState::REHYDRATING:
    data["currentState"] = "REHYDRATING";
    break;
  case SystemState::HEATING:
    data["currentState"] = "HEATING";
    break;
  case SystemState::MIXING:
    data["currentState"] = "MIXING";
    break;
  case SystemState::REFILLING:
    data["currentState"] = "REFILLING";
    break;
  case SystemState::EXTRACTING:
    data["currentState"] = "EXTRACTING";
    break;
  case SystemState::LOGGING:
    data["currentState"] = "LOGGING";
    break;
  case SystemState::PAUSED:
    data["currentState"] = "PAUSED";
    break;
  case SystemState::ENDED:
    data["currentState"] = "ENDED";
    break;
  case SystemState::ERROR:
    data["currentState"] = "ERROR";
    break;
  default:
    data["currentState"] = "UNKNOWN";
    break;
  }
  JsonObject parameters = data["parameters"].to<JsonObject>();
  parameters["volumeAddedPerCycle"] = volumeAddedPerCycle;
  parameters["syringeDiameter"] = syringeDiameter;
  parameters["desiredHeatingTemperature"] = desiredHeatingTemperature;
  parameters["durationOfHeating"] = durationOfHeating;
  parameters["durationOfMixing"] = durationOfMixing;
  parameters["numberOfCycles"] = numberOfCycles;
  parameters["syringeStepCount"] = syringeStepCount;
  parameters["heatingStartTime"] = heatingStartTime;
  parameters["heatingStarted"] = heatingStarted;
  parameters["mixingStartTime"] = mixingStartTime;
  parameters["mixingStarted"] = mixingStarted;
  parameters["completedCycles"] = completedCycles;
  parameters["currentCycle"] = currentCycle;
  parameters["heatingProgress"] = heatingProgressPercent;
  parameters["mixingProgress"] = mixingProgressPercent;
  JsonArray zones = parameters["sampleZonesToMix"].to<JsonArray>();
  for (int i = 0; i < sampleZoneCount; i++)
  {
    zones.add(sampleZonesArray[i]);
  }
  // Send to server
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  Serial.println("[WS] Sent ESP recovery packet to server");
}

// CYCLE PROGRESS COMMUNICATION

// static bool refillingStarted = false;
// void sendSyringeResetInfo()
// {
//   ArduinoJson::JsonDocument doc;
//   doc["type"] = "syringeReset";
//   doc["steps"] = syringeStepCount;

//   String message;
//   serializeJson(doc, message);
//   webSocket.sendTXT(message);

//   Serial.println("[WS] Sent syringe reset info");
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
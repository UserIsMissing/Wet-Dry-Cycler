#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "HEATING.h"
#include "globals.h"
#include "send_functions.h"
#include "REHYDRATION.h"

// Use globals from globals.h
extern WebSocketsClient webSocket;
extern unsigned long heatingStartTime;
extern float durationOfHeating;
extern unsigned long mixingStartTime;
extern float durationOfMixing;
extern int numberOfCycles;
extern int completedCycles;
extern SystemState currentState;

void sendHeartbeat()
{
  ArduinoJson::JsonDocument doc;
  doc["type"] = "heartbeat";
  doc["value"] = 1;
  char buffer[64];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("Sent heartbeat packet to frontend.");
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
  float percentUsed = (float)syringeStepCount / (float)MAX_SYRINGE_STEPS * 100.0;

  ArduinoJson::JsonDocument doc;
  doc["type"] = "syringePercentage";
  doc["value"] = percentUsed;

  char buffer[100];
  serializeJson(doc, buffer);
  webSocket.sendTXT(buffer);
  Serial.printf("[WS] Sent syringe percentage: %.2f%%\n", percentUsed);
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

    ArduinoJson::JsonDocument doc;
    doc["type"] = "currentState";
    doc["value"] = stateStr;

    char buffer[100];
    serializeJson(doc, buffer);
    webSocket.sendTXT(buffer);
    Serial.printf("[WS] Sent current state: %s\n", stateStr);
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
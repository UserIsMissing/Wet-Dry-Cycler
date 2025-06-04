#pragma once
#include <ArduinoJson.h>
#include <WString.h>

void handleStateCommand(const String &name, const String &state);
void handleRecoveryPacket(const JsonObject &data);
void handleParametersPacket(const JsonObject &parameters);

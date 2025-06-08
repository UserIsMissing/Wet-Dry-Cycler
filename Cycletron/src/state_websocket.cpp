#include "state_websocket.h"
#include "globals.h"
#include "send_functions.h"
#include "handle_functions.h"
#include "HEATING.h"
#include "MIXING.h"

constexpr unsigned int hash(const char *str, int h = 0) {
    return !str[h] ? 5381 : (hash(str, h + 1) * 33) ^ str[h];
}

unsigned long pausedElapsedTime = 0;
unsigned long pausedAtTime = 0;

/**
 * @brief State transition manager that handles motor control and timing logic
 *
 * Manages pause/resume timing, motor states during transitions, and ensures
 * proper state history tracking. Also handles stopping motors when transitioning
 * to paused states and sends state updates to the client.
 *
 * @param newState The state to transition to
 */
void setState(SystemState newState)
{
    // --- PAUSE/RESUME LOGIC ---
    if (newState == SystemState::PAUSED ||
        newState == SystemState::EXTRACTING ||
        newState == SystemState::REFILLING)
    {
        pausedAtTime = millis();
        // Save elapsed time for the current state
        if (currentState == SystemState::HEATING)
        {
            pausedElapsedTime = millis() - heatingStartTime;
        }
        else if (currentState == SystemState::MIXING)
        {
            pausedElapsedTime = millis() - mixingStartTime;
        }
        else
        {
            pausedElapsedTime = 0;
        }
    }
    // If resuming from PAUSED, EXTRACTING, or REFILLING, adjust start times and remaining durations
    if ((currentState == SystemState::PAUSED ||
         currentState == SystemState::EXTRACTING ||
         currentState == SystemState::REFILLING) &&
        (newState != SystemState::PAUSED &&
         newState != SystemState::EXTRACTING &&
         newState != SystemState::REFILLING))
    {
        if (previousState == SystemState::HEATING)
        {
            heatingStartTime = millis() - pausedElapsedTime;
            heatingDurationRemaining -= pausedElapsedTime;
            if (heatingDurationRemaining < 0)
                heatingDurationRemaining = 0;
        }
        else if (previousState == SystemState::MIXING)
        {
            mixingStartTime = millis() - pausedElapsedTime;
            mixingDurationRemaining -= pausedElapsedTime;
            if (mixingDurationRemaining < 0)
                mixingDurationRemaining = 0;
        }
        pausedElapsedTime = 0;
        pausedAtTime = 0;
    }
    // --- END PAUSE/RESUME LOGIC ---

    // Stop motors if transitioning to PAUSED state
    if (newState == SystemState::PAUSED ||
        newState == SystemState::EXTRACTING ||
        newState == SystemState::ENDED ||
        newState == SystemState::REFILLING)
    {
        if (currentState == SystemState::HEATING)
        {
            HEATING_Off();
            heatingStarted = false;
            Serial.println("[PAUSED] Mixing motors stopped due to state transition");
        }
        else if (currentState == SystemState::MIXING)
        {
            MIXING_AllMotors_Off();
            mixingStarted = false;
            Serial.println("[PAUSED] Motors stopped due to state transition");
        }
    }

    if (currentState != SystemState::PAUSED &&
        currentState != SystemState::REFILLING &&
        currentState != SystemState::EXTRACTING)
    {
        previousState = currentState;
    }
    currentState = newState;
    sendCurrentState();
}

/**
 * @brief WebSocket event handler for managing client communication
 *
 * Processes connection events and parses incoming JSON messages for:
 * - Connection management (connect/disconnect)
 * - Vial setup commands
 * - System state recovery
 * - Parameter updates
 * - General state commands
 *
 * @param type WebSocket event type
 * @param payload Message data buffer
 * @param length Length of payload data
 */
void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_CONNECTED:
        Serial.println("WebSocket connected");
        {
            ArduinoJson::DynamicJsonDocument doc(256); // Ensure proper scope
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
        ArduinoJson::DynamicJsonDocument doc(512);
        auto err = deserializeJson(doc, payload, length);
        if (err)
        {
            Serial.print("JSON parse failed: ");
            Serial.println(err.c_str());
            break;
        }

        // Use switch for message type handling
        String msgType = doc["type"].as<String>();
        switch (hash(msgType.c_str())){

        case hash("espRecoveryState"):
            if (doc["data"].is<JsonObject>())
            {
                handleRecoveryPacket(doc["data"].as<JsonObject>());
            }
            break;

        case hash("parameters"):
            if (doc["data"].is<JsonObject>())
            {
                if (currentState == SystemState::WAITING)
                {
                    handleParametersPacket(doc["data"].as<JsonObject>());
                }
                else
                {
                    Serial0.printf("[PARAMETERS] Ignoring packet in state: %d\n",
                                   static_cast<int>(currentState));
                }
            }
            break;
        default:
            // Handle state command format
            if (doc["name"].is<const char *>() && doc["state"].is<const char *>())
            {
                handleStateCommand(
                    doc["name"].as<String>(),
                    doc["state"].as<String>());
            }
            else
            {
                Serial.println("Invalid packet format");
            }
            break;
        }
    }
    break;

    default:
        break;
    }
}
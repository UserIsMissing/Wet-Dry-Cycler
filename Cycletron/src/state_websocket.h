#ifndef STATE_WEBSOCKET_H
#define STATE_WEBSOCKET_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include "globals.h"

extern WebSocketsClient webSocket;

/**
 * @brief Updates the system state and handles state transitions
 * 
 * Manages motor states, timing, and pause/resume functionality
 * during state transitions. Also sends state updates to the client.
 * 
 * @param newState The state to transition to
 */
void setState(SystemState newState);

/**
 * @brief Handles WebSocket events and incoming messages
 * 
 * Processes connection events and incoming JSON messages for
 * state changes, parameter updates, and system recovery.
 * 
 * @param type The type of WebSocket event
 * @param payload The message data
 * @param length Length of the payload
 */
void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);

/**
 * @brief Hash function for string literals
 * 
 * @param str The string to hash
 * @param h The initial hash value
 * @return The computed hash value
 */
constexpr unsigned int hash(const char* str, int h);

#endif // STATE_WEBSOCKET_H

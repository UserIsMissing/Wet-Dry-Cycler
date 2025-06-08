#pragma once
#include <ArduinoJson.h>
#include <WString.h>

/**
 * @brief Enumeration of supported command types from front-end interface.
 *
 * These commands control the system state and behavior based on user input.
 */
enum class CommandType {
    VIAL_SETUP,       ///< Command to begin vial preparation
    START_CYCLE,      ///< Command to begin wet-dry cycle
    PAUSE_CYCLE,      ///< Command to pause/resume the current cycle
    END_CYCLE,        ///< Command to terminate the current cycle
    EXTRACT,          ///< Command to initiate fluid extraction
    REFILL,           ///< Command to refill the syringe
    LOG_CYCLE,        ///< Command to enter data logging mode
    RESTART_ESP32,    ///< Command to restart the ESP32
    UNKNOWN           ///< Fallback for unrecognized commands
};

/**
 * @brief Converts a string command to its associated CommandType enum.
 *
 * @param name Command string (e.g., "startCycle")
 * @return Corresponding CommandType value
 */
CommandType parseCommand(const String &name);

/**
 * @brief Processes incoming command and state from the front-end interface.
 *
 * Handles state transitions and initiates related system behavior.
 *
 * @param name Command name (e.g., "vialSetup")
 * @param state Desired command state (e.g., "yes", "on")
 */
void handleStateCommand(const String &name, const String &state);

/**
 * @brief Restores internal state from previously saved recovery JSON.
 *
 * Used after a reboot to continue operation from the last known state.
 *
 * @param data JSON object with saved system state and parameters
 */
void handleRecoveryPacket(const JsonObject &data);

/**
 * @brief Parses and stores configuration parameters sent from the front-end.
 *
 * Updates internal global variables used for operational setup.
 *
 * @param parameters JSON object containing the parameters
 */
void handleParametersPacket(const JsonObject &parameters);

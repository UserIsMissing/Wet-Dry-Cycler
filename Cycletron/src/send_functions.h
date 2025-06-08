#pragma once
/**
 * @file    REHYDRATION.h
 * @brief   WebSocket communication functions for system status updates
 *
 * This header declares functions that send status updates, progress, and 
 * notifications to the frontend via WebSocket. Handles temperature readings,
 * cycle progress, state changes, and other telemetry data.
 *
 * Designed for real-time monitoring of wet-dry cycling systems.
 *
 * Author: Rafael Delwart
 * Date:   May 2025
 */

#ifndef SEND_FUNCTIONS_H
#define SEND_FUNCTIONS_H

/**
 * @brief Sends heartbeat packet to frontend
 * 
 * Periodic message to confirm system is alive and responsive
 */
void sendHeartbeat();

/**
 * @brief Sends current temperature reading to frontend
 * 
 * Reports temperature from heating system for monitoring
 */
void sendTemperature();

/**
 * @brief Sends syringe fluid level to frontend
 * 
 * Reports percentage of fluid remaining in syringe
 */
void sendSyringePercentage();

/**
 * @brief Sends heating progress to frontend
 * 
 * Reports percentage completion of heating cycle
 */
void sendHeatingProgress();

/**
 * @brief Sends mixing progress to frontend
 * 
 * Reports percentage completion of mixing cycle
 */
void sendMixingProgress();

/**
 * @brief Sends cycle progress to frontend
 * 
 * Reports current cycle number and total progress
 */
void sendCycleProgress();

/**
 * @brief Sends end of cycles notification
 * 
 * Notifies frontend that all cycles are complete
 */
void sendEndOfCycles();

/**
 * @brief Sends syringe reset confirmation
 * 
 * Reports when syringe position has been reset
 */
void sendSyringeResetInfo();

/**
 * @brief Sends current system state to frontend
 * 
 * Reports state machine transitions and current status
 */
void sendCurrentState();

/**
 * @brief Sends extraction ready notification to frontend
 * 
 * Notifies the frontend that the system is ready for extraction
 * after completing a cycle or reaching extraction point
 */
void sendExtractionReady();

#endif // SEND_FUNCTIONS_H

/**
 * @file    MOVEMENT.h
 * @brief   Movement subsystem module for stepper motor navigation
 *
 * This module provides initialization and control routines for a stepper
 * motor used to navigate between zones (heating, mixing, extraction, etc.).
 * It interfaces with bumpers, DRV8825 driver, and handles initialization,
 * direction correction, and safe movement routines.
 *
 * @author  Cole Schreiner
 * @date    14 Apr 2025 adapted for ESP 13 May 2025
 */

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include <Arduino.h>
#include "DRV8825.h"

// === Global State Flag ===
/**
 * @brief Global variable indicating bumper contact:
 * - 0 = no bumper pressed
 * - 1 = front bumper pressed
 * - 2 = back bumper pressed
 */
extern int BUMPER_STATE;

// === Bumper Pin Struct ===
/**
 * @struct BUMPER_t
 * @brief  Contains GPIO pin assignments for bumpers and control buttons.
 */
typedef struct
{
    int front_bumper_pin; ///< GPIO pin for front bumper
    int back_bumper_pin;  ///< GPIO pin for back bumper
} BUMPER_t;

// === API Functions ===

/**
 * @brief Initializes the movement motor and immediately disables it.
 *
 * Use this to ensure the motor is set up but not powered.
 */
void MOVEMENT_InitAndDisable();

/**
 * @brief Initializes the MOVEMENT module.
 *
 * This function:
 * - Initializes the DRV8825 stepper driver.
 * - Sets up bumper state using digital reads.
 * - Moves the motor slightly back or forward based on bumper detection.
 * - Ensures the system starts in a known alignment.
 */
void MOVEMENT_Init(void);

/**
 * @brief Moves the motor a small amount in reverse to “unstick” the system.
 *
 * This function is typically used during initialization to apply a short
 * backward or forward motion to free the mechanism before larger moves.
 *
 * @param InitialSmallSteps Number of microsteps to take
 * @param UndoDirection Direction (DRV8825_FORWARD or DRV8825_BACKWARD)
 */
void MOVEMENT_First_Steps(int InitialSmallSteps, int UndoDirection);

/**
 * @brief Checks for DRV8825 fault condition.
 *
 * Reads the fault pin of the DRV8825 driver and returns a boolean indicating
 * if the driver is in a fault state (e.g., overcurrent, undervoltage, etc.).
 *
 * @param motor Pointer to the configured DRV8825 motor
 * @return 1 if fault is detected, 0 otherwise
 */
int CheckFAULT(DRV8825_t *motor);

/**
 * @brief Checks bumper states.
 *
 * Reads bumper GPIOs and updates the global `BUMPER_STATE` variable.
 * Helps determine if the motor has reached a physical limit.
 *
 * @return 1 if front bumper pressed, 2 if back bumper pressed, 0 if none
 */
int CheckBumpers(void);

/**
 * @brief Runs the full movement procedure based on bumper state.
 *
 * Uses the DRV8825 motor driver to move toward the opposite bumper.
 * Checks for faults and bumpers continuously to ensure safe stopping.
 *
 * This function supports both forward and backward travel depending
 * on which bumper is pressed when movement begins.
 */
void MOVEMENT_Move(void);



 /**
  * @brief Moves motor based on bumper state.
  *
  * If front bumper is pressed, move backward until back is pressed.
  * If back bumper is pressed, move forward until front is pressed.
  * Movement stops immediately if target bumper is hit.
  */
 void MOVEMENT_Move_FORWARD();


 void MOVEMENT_Move_BACKWARD();

/**
 * @brief Immediately stops the motor.
 *
 * Pulls the step pin LOW and optionally disables the driver.
 * Should be called after reaching a bumper or fault.
 */
void MOVEMENT_Stop(void);

/**
 * @brief Interrupt handler for front bumper trigger.
 *
 * Sets the global movementFrontTriggered flag to true.
 */

void IRAM_ATTR onMovementFrontLimit();

/**
 * @brief Interrupt handler for back bumper trigger.
 *
 * Sets a flag indicating the back bumper has been triggered.
 */
void IRAM_ATTR onMovementBackLimit();

/**
 * @brief Configures GPIO pins for bumpers and sets up interrupts.
 *
 * Sets the front and back bumper pins as INPUT_PULLUP and attaches
 * interrupt handlers for both bumpers.
 */
void MOVEMENT_ConfigureInterrupts();

/**
 * @brief Handles interrupts for front and back bumpers.
 *
 * Checks the global flags and stops the motor if either bumper is triggered.
 */
void MOVEMENT_HandleInterrupts();

#endif // MOVEMENT_H

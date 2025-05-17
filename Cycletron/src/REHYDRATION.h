/**
 * @file    REHYDRATION.h
 * @brief   Syringe pump control interface for RNA fluid rehydration
 *
 * This header declares the control functions for a stepper motor
 * used to precisely dispense and retract fluid via a syringe pump.
 * It uses a DRV8825 stepper motor driver to move the plunger and
 * convert microliter requests into accurate step commands.
 *
 * Designed for use in wet-dry cycling systems for RNA research.
 *
 * Author: Rafael Delwart
 * Date:   20 Apr 2025 adapted for ESP 13 May 2025
 */

#ifndef REHYDRATION_H
#define REHYDRATION_H

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



/**
 * @brief Initializes the syringe pump stepper motor.
 *
 * This sets up the DRV8825 driver pins, applies the default microstepping mode,
 * and prints calibration data including the volume of fluid dispensed per motor step.
 * Call this once during system setup.
 *
 * @param syringeDiameterInches Syringe diameter in inches (e.g., 0.25 for 1/4 inch)
 */
void Rehydration_Init(float syringeDiameterInches);

/**
 * @brief Dispenses a specific volume of liquid in microliters by pushing the plunger forward.
 *
 * Converts the desired microliters to motor steps based on syringe and leadscrew dimensions,
 * and moves the motor forward (push) at a controlled speed to ensure accurate fluid delivery.
 *
 * @param uL Volume of fluid to dispense, in microliters.
 * @param syringeDiameterInches Syringe diameter in inches (e.g., 0.25 for 1/4 inch)
 */
void Rehydration_Push(uint32_t uL, float syringeDiameterInches);

/**
 * @brief Retracts the syringe plunger to draw fluid or prime the system.
 *
 * Similar to Rehydration_Push(), but moves in reverse to create suction.
 * Useful for loading liquid into the syringe or preparing it for operation.
 *
 * @param uL Volume of fluid to draw back, in microliters.
 * @param syringeDiameterInches Syringe diameter in inches (e.g., 0.25 for 1/4 inch)
 */
void Rehydration_Pull(uint32_t uL, float syringeDiameterInches);

/**
 * @brief Disables the stepper motor to stop motion and reduce power consumption.
 *
 * This function cuts power to the DRV8825 driver, halting the motor and preventing
 * unnecessary heating or energy drain. It should be called after movement is complete.
 */
void Rehydration_Stop();

// BUMPER HANDLING

/**
 * @brief Interrupt handler for front bumper trigger.
 *
 * Sets a flag indicating the front bumper has been triggered.
 */
void IRAM_ATTR onRehydrationFrontLimit();
/**
 * @brief Interrupt handler for back bumper trigger.
 *
 * Sets a flag indicating the back bumper has been triggered.
 */
void IRAM_ATTR onRehydrationBackLimit();

/**
 * @brief Configures GPIO pins for front and back bumpers.
 *
 * Sets the specified pins as input with pull-up resistors and attaches
 * interrupt handlers for detecting bumper presses.
 *
 */
void REHYDRATION_ConfigureInterrupts();
/**
 * @brief Handles interrupts for front and back bumpers.
 *
 * Handles the state of the bumpers and stops the rehydration process
 * if either bumper is triggered. This function should be called in the
 * main loop to check for bumper events.
 *
 */

void REHYDRATION_HandleInterrupts();

#endif // REHYDRATION_H

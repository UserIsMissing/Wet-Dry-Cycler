/**
 * @file    MOVEMENT.cpp
 * @brief   Movement module for stepper navigation using DRV8825 driver
 *
 * This module controls stepper motor movement across subsystems (e.g., heating,
 * mixing, extraction). It includes bumper detection, safe startup handling,
 * and DRV8825 stepper driver integration.
 *
 * Author: Cole Schreiner
 * Date:   14 Apr 2025 adapted for ESP 13 May 2025
 */

#include <Arduino.h>
#include "MOVEMENT.h"
#include "globals.h"

// === Constants ===
#define MOVEMENT_STEP_DELAY_US 1000 // Delay between microsteps

// === Global State ===
volatile bool movementFrontTriggered = false;
volatile bool movementBackTriggered = false;

// === Motor and Sensor Config ===
DRV8825_t movementMotor = {
    .step_pin = 6,
    .dir_pin = 7,
    .fault_pin = 15,
    .mode0_pin = 16,
    .mode1_pin = 17,
    .mode2_pin = 18,
    .enable_pin = 8};

BUMPER_t bumpers_m = {
    .front_bumper_pin = 3,
    .back_bumper_pin = 10,
};

/**
 * @brief Applies small reverse motion to unstuck the mechanism.
 *
 * Used during initialization to nudge the system out of an uncertain
 * state before entering normal operation.
 */
void MOVEMENT_First_Steps(int InitialSmallSteps, int UndoDirection)
{
  DRV8825_Set_Step_Mode(&movementMotor, DRV8825_HALF_STEP); // Use half-step for precision
  DRV8825_Move(&movementMotor, InitialSmallSteps, UndoDirection, MOVEMENT_STEP_DELAY_US);
  DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP); // Return to full step mode
}

/**
 * @brief Initializes the movement motor and immediately disables it.
 *
 * Use this to ensure the motor is set up but not powered.
 *
 * .
 */
void MOVEMENT_InitAndDisable()
{
  DRV8825_Init(&movementMotor);
  Serial.println("[MOVEMENT] Motor initialized and disabled.");
}

/**
 * @brief Initializes the MOVEMENT system.
 *
 * Calibrates motor direction using bumper input and prepares the system
 * for safe directional control. It attempts to find the front bumper
 * and align to it before stopping.
 */
void MOVEMENT_Init()
{
    delay(500); // Delay for system stability

    DRV8825_Init(&movementMotor); // Initialize motor driver
    CheckBumpers();               // Read initial bumper state

    Serial.printf("Initial BUMPER_STATE: %d\n", BUMPER_STATE);

    // Ensure no movement if the back bumper is already pressed
    if (digitalRead(bumpers_m.back_bumper_pin) == HIGH)
    {
        Serial.println("[MOVEMENT] Back bumper already pressed. No movement required.");
        DRV8825_Disable(&movementMotor);
    }
    else
    {
        while (BUMPER_STATE != 2)
        {
            DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US);
            CheckBumpers();
        }
        DRV8825_Disable(&movementMotor);
    }

    Serial.println("[MOVEMENT] Initialization complete.");
}

/**
 * @brief Reads bumper interrupt flags and updates global BUMPER_STATE.
 *
 * @return 1 = front bumper triggered, 2 = back bumper triggered, 0 = none
 */
int CheckBumpers()
{
  if (movementFrontTriggered)
  {
    movementFrontTriggered = false; // Reset flag
    BUMPER_STATE = 1;
    Serial.println("[MOVEMENT] Front bumper triggered.");
    return 1;
  }
  if (movementBackTriggered)
  {
    movementBackTriggered = false; // Reset flag
    BUMPER_STATE = 2;
    Serial.println("[MOVEMENT] Back bumper triggered.");
    return 2;
  }
  BUMPER_STATE = 0;
  return 0;
}

/**
 * @brief Moves motor based on bumper state.
 *
 * If front bumper is pressed, move backward until back is pressed.
 * If back bumper is pressed, move forward until front is pressed.
 * Movement stops immediately if target bumper is hit.
 */
void MOVEMENT_Move_FORWARD()
{
  // MOVEMENT_First_Steps(); //may need to add to unstick mechanism
  DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP);
  CheckBumpers();
  while (BUMPER_STATE != 1)
  {
    DRV8825_Move(&movementMotor, 1, DRV8825_FORWARD, MOVEMENT_STEP_DELAY_US);
    CheckBumpers();
  }
  MOVEMENT_Stop();
}

void MOVEMENT_Move_BACKWARD()
{
  // MOVEMENT_First_Steps(); //may need to add to unstick mechanism
  DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP);
  CheckBumpers();

  while (BUMPER_STATE != 2)
  {
    DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US);
    CheckBumpers();
  }
  MOVEMENT_Stop();
}

/**
 * @brief Stops the motor by setting step pin LOW.
 */
void MOVEMENT_Stop()
{
  digitalWrite(movementMotor.step_pin, LOW);
  DRV8825_Disable(&movementMotor);
  Serial.println("[MOVEMENT] Motor stopped.");
}

/**
 * @brief Interrupt handler for front bumper trigger.
 *
 * Sets the global movementFrontTriggered flag to true.
 */
void IRAM_ATTR onMovementFrontLimit()
{
  static volatile unsigned long lastFrontInterruptTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastFrontInterruptTime > 50)
  { // Debounce threshold: 50 ms
    movementFrontTriggered = true;
    lastFrontInterruptTime = currentTime;
  }
}

/**
 * @brief Interrupt handler for back bumper trigger.
 *
 * Sets a flag indicating the back bumper has been triggered.
 */
void IRAM_ATTR onMovementBackLimit()
{
  static volatile unsigned long lastBackInterruptTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastBackInterruptTime > 50)
  { // Debounce threshold: 50 ms
    movementBackTriggered = true;
    lastBackInterruptTime = currentTime;
  }
}

/**
 * @brief Configures GPIO pins for bumpers and sets up interrupts.
 *
 * Sets the front and back bumper pins as INPUT_PULLUP and attaches
 * interrupt handlers for both bumpers.
 *
 * Note: This version skips `detachInterrupt()` to avoid crashing if
 * the ISR service hasn't been initialized yet. It only attaches if
 * the pin is HIGH (i.e., unpressed).
 */
void MOVEMENT_ConfigureInterrupts()
{
  pinMode(bumpers_m.front_bumper_pin, INPUT_PULLDOWN);
  pinMode(bumpers_m.back_bumper_pin, INPUT_PULLDOWN);

  movementFrontTriggered = false;
  movementBackTriggered = false;

  attachInterrupt(digitalPinToInterrupt(bumpers_m.front_bumper_pin), onMovementFrontLimit, RISING);
  attachInterrupt(digitalPinToInterrupt(bumpers_m.back_bumper_pin), onMovementBackLimit, RISING);
}
/**
 * @brief Handles interrupts for front and back bumpers.
 *
 * Checks the global flags and stops the motor if either bumper is triggered.
 * Interrupts are detached temporarily to prevent repeated firing and
 * re-attached only if the pin returns to HIGH state.
 */
void MOVEMENT_HandleInterrupts()
{
  if (movementFrontTriggered)
  {
    movementFrontTriggered = false;
    detachInterrupt(digitalPinToInterrupt(bumpers_m.front_bumper_pin));
    attachInterrupt(digitalPinToInterrupt(bumpers_m.front_bumper_pin), onMovementFrontLimit, FALLING);
  }

  if (movementBackTriggered)
  {
    movementBackTriggered = false;
    detachInterrupt(digitalPinToInterrupt(bumpers_m.back_bumper_pin));
    attachInterrupt(digitalPinToInterrupt(bumpers_m.back_bumper_pin), onMovementBackLimit, FALLING);
  }
}
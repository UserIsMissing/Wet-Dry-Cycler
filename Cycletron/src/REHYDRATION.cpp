/**
 * @file    REHYDRATION.cpp
 * @brief   Syringe pump control for RNA fluid rehydration
 *
 * This module drives a DRV8825-controlled stepper motor attached to a syringe pump,
 * converting volume-based commands into precise motor steps for pushing and pulling fluids.
 * Designed for use in wet-dry cycling applications such as RNA rehydration.
 *
 * Author: Rafael Delwart
 * Date:   20 Apr 2025 adapted for ESP 13 May 2025
 */

#include <Arduino.h>
#include "REHYDRATION.h"
#include "MOVEMENT.h"
#include "DRV8825.h"
#include <math.h>

volatile bool rehydrationFrontTriggered = false;
volatile bool rehydrationBackTriggered = false;

// === Global State ===
int BUMPER_STATE = 0;

// === Motor Configuration ===
DRV8825_t rehydrationMotor = {
    .step_pin = 1,
    .dir_pin = 2,
    .fault_pin = 42,
    .mode0_pin = 41,
    .mode1_pin = 40,
    .mode2_pin = 39,
    .enable_pin = 38};

BUMPER_t bumpers_r = {
    .front_bumper_pin = 3,
    .back_bumper_pin = 10,
};



/**
 * @brief Calculates how many microliters are moved per motor step.
 *
 * This function computes the volume displaced by one step of the leadscrew-driven
 * syringe pump based on the cross-sectional area of the syringe and the travel
 * distance per motor step.
 *
 * @param syringeDiameterInches Inner diameter of the syringe barrel (in inches).
 * @return Volume in microliters (µL) moved by one motor step.
 */
float calculate_uL_per_step(float syringeDiameterInches)
{
    // Convert diameter to radius
    float radius_in = syringeDiameterInches / 2.0f;
    // Cross-sectional area of the syringe (in²)
    float area_in2 = M_PI * radius_in * radius_in;
    // Travel distance per motor step (inches)
    float step_travel_in = LEADSCREW_TRAVEL_IN_PER_REV / TOTAL_STEPS_PER_REV;
    // Volume displaced per step (in³)
    float volume_in3 = area_in2 * step_travel_in;
    // Convert to microliters
    float volume_uL = volume_in3 * INCH3_TO_UL;
    return volume_uL;
}

/**
 * @brief Initializes the syringe pump motor and immediately disables it.
 *
 * Use this to ensure the motor is set up but not powered.
 *
 * @param syringeDiameterInches Syringe inner diameter in inches.
 */
void Rehydration_InitAndDisable()
{
    DRV8825_Init(&rehydrationMotor);
    Serial.println("[REHYDRATION] Motor initialized and disabled.");
}


/**
 * @brief Initializes the syringe pump motor and prints calibration data.
 *
 * This function configures the DRV8825 motor with a default microstepping mode,
 * converts the provided syringe diameter from inches to millimeters, calculates
 * the microliters-per-step constant, and displays debug info.
 *
 * @param syringeDiameterInches Syringe inner diameter in inches (e.g., 0.25 for 1/4 inch)
 */
void Rehydration_Init(float syringeDiameterInches)
{
    float syringeDiameterMM = syringeDiameterInches * 25.4f;

    DRV8825_Init(&rehydrationMotor);
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);

    float uL_per_step = calculate_uL_per_step(syringeDiameterInches);

    Serial.println("[REHYDRATION] Motor initialized.");
    Serial.printf("[REHYDRATION] Syringe diameter: %.2f in (%.2f mm)\n", syringeDiameterInches, syringeDiameterMM);
    Serial.printf("[REHYDRATION] uL per step = %.5f\n", uL_per_step);
}



/**
 * @brief Dispenses fluid by pushing the syringe plunger forward.
 *
 * Converts microliters into steps and sends a movement command.
 *
 * @param uL Volume to dispense in microliters
 * @param syringeDiameterInches Syringe diameter in inches
 */
void Rehydration_Push(uint32_t uL, float syringeDiameterInches)
{
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);

    float uL_per_step = calculate_uL_per_step(syringeDiameterInches);
    uint32_t steps = (uint32_t)(uL / uL_per_step);

    Serial.printf("[REHYDRATION] Pushing %lu uL (%lu steps)\n", uL, steps);
    DRV8825_Move(&rehydrationMotor, steps, DRV8825_FORWARD, 500); // Push plunger
}

/**
 * @brief Pulls fluid into the syringe (or primes it).
 *
 * Moves the motor in reverse to retract plunger and draw fluid.
 *
 * @param uL Volume to draw back in microliters
 * @param syringeDiameterInches Syringe diameter in inches
 */
void Rehydration_Pull(uint32_t uL, float syringeDiameterInches)
{
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_QUARTER_STEP); // More control

    float uL_per_step = calculate_uL_per_step(syringeDiameterInches);
    uint32_t steps = (uint32_t)(uL / uL_per_step);

    Serial.printf("[REHYDRATION] Retracting %lu uL (%lu steps)\n", uL, steps);
    DRV8825_Move(&rehydrationMotor, steps, DRV8825_BACKWARD, DRV8825_DEFAULT_STEP_DELAY_US);
}

/**
 * @brief Stops and disables the syringe pump motor.
 *
 * Use this after completing fluid movement to reduce heat and power draw.
 */
void Rehydration_Stop()
{
    DRV8825_Disable(&rehydrationMotor);
    Serial.println("[REHYDRATION] Motor stopped.");
}

// === BUMPER INTERRUPTS ===

/**
 * @brief Interrupt handler for front bumper trigger.
 *
 * Sets a flag indicating the front bumper has been triggered.
 * Keep this minimal to avoid watchdog resets.
 */
void IRAM_ATTR onRehydrationFrontLimit() {
    rehydrationFrontTriggered = true;
}

/**
 * @brief Interrupt handler for back bumper trigger.
 *
 * Sets a flag indicating the back bumper has been triggered.
 * Keep this minimal to avoid watchdog resets.
 */
void IRAM_ATTR onRehydrationBackLimit() {
    rehydrationBackTriggered = true;
}

/**
 * @brief Configures GPIO pins for front and back bumpers.
 *
 * Sets the specified pins as input with pull-up resistors and attaches
 * interrupt handlers for detecting bumper presses.
 * 
 * Interrupts are only attached if the pin is HIGH (not already pressed)
 * to avoid immediate triggering on startup from floating or LOW states.
 */
void REHYDRATION_ConfigureInterrupts() {
    // Configure GPIO pins with pull-up resistors
    pinMode(bumpers_r.front_bumper_pin, INPUT_PULLUP);
    pinMode(bumpers_r.back_bumper_pin, INPUT_PULLUP);

    // Clear any previous state
    rehydrationFrontTriggered = false;
    rehydrationBackTriggered = false;

    // Attach front bumper ISR if the pin is not LOW on boot
    if (digitalRead(bumpers_r.front_bumper_pin) == HIGH) {
        attachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin), onRehydrationFrontLimit, FALLING);
    } else {
        Serial.println("[REHYDRATION] Skipping front interrupt attach — pin LOW despite pullup");
    }

    // Attach back bumper ISR if the pin is not LOW on boot
    if (digitalRead(bumpers_r.back_bumper_pin) == HIGH) {
        attachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin), onRehydrationBackLimit, FALLING);
    } else {
        Serial.println("[REHYDRATION] Skipping back interrupt attach — pin LOW despite pullup");
    }
}

/**
 * @brief Handles bumper-triggered interrupts.
 *
 * This function should be called in the main loop.
 * It disables and re-enables interrupts based on pin state
 * to prevent rapid re-triggering. Also stops the motor.
 */
void REHYDRATION_HandleInterrupts() {
    if (rehydrationFrontTriggered) {
        rehydrationFrontTriggered = false;

        // Detach interrupt to prevent bouncing or re-entry
        detachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin));
        Serial.println("[INTERRUPT] Rehydration front limit triggered");
        Rehydration_Stop();

        // Re-attach only if the pin is HIGH again
        if (digitalRead(bumpers_r.front_bumper_pin) == HIGH) {
            attachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin), onRehydrationFrontLimit, FALLING);
        }
    }

    if (rehydrationBackTriggered) {
        rehydrationBackTriggered = false;

        // Detach interrupt to prevent bouncing or re-entry
        detachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin));
        Serial.println("[INTERRUPT] Rehydration back limit triggered");
        Rehydration_Stop();

        // Re-attach only if the pin is HIGH again
        if (digitalRead(bumpers_r.back_bumper_pin) == HIGH) {
            attachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin), onRehydrationBackLimit, FALLING);
        }
    }
}

/**
 * @brief Continuously moves the syringe backward until the back bumper is triggered.
 *
 * This uses a while-loop to move indefinitely in the BACKWARD direction
 * until the back limit switch is hit (rehydrationBackTriggered becomes true).
 * Be sure REHYDRATION_ConfigureInterrupts() has been called before using.
 */
void Rehydration_BackUntilBumper() {
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_QUARTER_STEP); // precise and slower

    rehydrationBackTriggered = false;
    Serial.println("[REHYDRATION] Moving backward until bumper is triggered...");

    while (!rehydrationBackTriggered) {
        DRV8825_Move(&rehydrationMotor, 1, DRV8825_BACKWARD, 500); // one step at a time
        delayMicroseconds(200); // small delay to allow interrupt detection
    }

    Rehydration_Stop();
    Serial.println("[REHYDRATION] Back bumper triggered — motion stopped.");
}
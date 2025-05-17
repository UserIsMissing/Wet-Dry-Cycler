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

// === Syringe & Motion Parameters ===
#define STEPPER_STEPS_PER_REV 200 // Full steps per revolution of stepper motor
#define MICROSTEPPING 16          // Microstepping mode (e.g., 1/16 step)
#define LEADSCREW_TPI 20          // Threads per inch of leadscrew
#define SYRINGE_DIAMETER_IN 1.0   // Syringe barrel inner diameter in inches

// === Conversion Factors ===
#define TOTAL_STEPS_PER_REV (STEPPER_STEPS_PER_REV * MICROSTEPPING)
#define LEADSCREW_TRAVEL_IN_PER_REV (1.0 / LEADSCREW_TPI)
#define STEP_TRAVEL_IN (LEADSCREW_TRAVEL_IN_PER_REV / TOTAL_STEPS_PER_REV)
#define INCH3_TO_UL 16387.064 // Conversion factor from in³ to microliters

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
static float calculate_uL_per_step(float syringeDiameterInches)
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

void IRAM_ATTR onRehydrationFrontLimit() {
    rehydrationFrontTriggered = true;
}

void IRAM_ATTR onRehydrationBackLimit() {
    rehydrationBackTriggered = true;
}

void REHYDRATION_ConfigureInterrupts() {
    // Set pins as INPUT_PULLUP
    pinMode(bumpers_r.front_bumper_pin, INPUT_PULLUP);
    pinMode(bumpers_r.back_bumper_pin, INPUT_PULLUP);

    rehydrationFrontTriggered = false;
    rehydrationBackTriggered = false;

    // Only attach if the pin is HIGH (i.e., not pressed)
    if (digitalRead(bumpers_r.front_bumper_pin) == HIGH) {
        attachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin), onRehydrationFrontLimit, FALLING);
    } else {
        Serial.println("[REHYDRATION] Skipping front interrupt attach — pin LOW despite pullup");
    }

    if (digitalRead(bumpers_r.back_bumper_pin) == HIGH) {
        attachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin), onRehydrationBackLimit, FALLING);
    } else {
        Serial.println("[REHYDRATION] Skipping back interrupt attach — pin LOW despite pullup");
    }
}

void REHYDRATION_HandleInterrupts() {
    if (rehydrationFrontTriggered) {
        rehydrationFrontTriggered = false;

        // Disable the interrupt safely before re-arming
        detachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin));
        Serial.println("[INTERRUPT] Rehydration front limit triggered");
        Rehydration_Stop();

        // Optional: reattach only if input is HIGH again
        if (digitalRead(bumpers_r.front_bumper_pin) == HIGH) {
            attachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin), onRehydrationFrontLimit, FALLING);
        }
    }

    if (rehydrationBackTriggered) {
        rehydrationBackTriggered = false;

        detachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin));
        Serial.println("[INTERRUPT] Rehydration back limit triggered");
        Rehydration_Stop();

        if (digitalRead(bumpers_r.back_bumper_pin) == HIGH) {
            attachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin), onRehydrationBackLimit, FALLING);
        }
    }
}

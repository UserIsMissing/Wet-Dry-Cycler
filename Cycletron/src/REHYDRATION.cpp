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
#include "globals.h"
#include "send_functions.h"
#include <math.h>

volatile bool rehydrationFrontTriggered = false;
volatile bool rehydrationBackTriggered = false;


// #define REHYDRATION_TEST
// #define REHYDRATION_TEST_2

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
    .front_bumper_pin = 46,
    .back_bumper_pin = 9,
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
    uL = uL * 0.909; //Scale factor to calibrate syringe pump
    float uL_per_step = calculate_uL_per_step(syringeDiameterInches);
    uint32_t steps = (uint32_t)(uL / uL_per_step);

    // Error check: will this exceed max steps?
    extern int syringeStepCount;
    if (syringeStepCount + (int)steps > MAX_SYRINGE_STEPS) {
        Serial.println("[ERROR] Syringe step count would exceed safe range! Aborting push.");
        currentState = SystemState::ERROR;
        sendSystemError(ERROR_SYRINGE_MAX_STEPS);
        return;
    }

    Serial.printf("[REHYDRATION] Pushing %lu uL (%lu steps)\n", uL, steps);
    DRV8825_Move(&rehydrationMotor, steps, DRV8825_FORWARD, 50); // Push plunger
    syringeStepCount += steps;
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

    // Error check: don't allow negative step count
    extern int syringeStepCount;
    if (syringeStepCount - (int)steps < 0) {
        Serial.println("[ERROR] Syringe step count would go negative! Aborting pull.");
        currentState = SystemState::ERROR;
        sendSystemError(ERROR_SYRINGE_MAX_STEPS);
        return;
    }

    Serial.printf("[REHYDRATION] Retracting %lu uL (%lu steps)\n", uL, steps);
    DRV8825_Move(&rehydrationMotor, steps, DRV8825_BACKWARD, DRV8825_DEFAULT_STEP_DELAY_US);
    syringeStepCount -= steps;
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
    pinMode(bumpers_r.front_bumper_pin, INPUT_PULLDOWN);
    pinMode(bumpers_r.back_bumper_pin, INPUT_PULLDOWN);

    // Clear any previous state
    rehydrationFrontTriggered = false;
    rehydrationBackTriggered = false;


        attachInterrupt(digitalPinToInterrupt(bumpers_r.front_bumper_pin), onRehydrationFrontLimit, RISING);

        attachInterrupt(digitalPinToInterrupt(bumpers_r.back_bumper_pin), onRehydrationBackLimit, RISING);
}

/**
 * @brief Handles bumper-triggered interrupts.
 *
 * This function should be called in the main loop.
 * It disables and re-enables interrupts based on pin state
 * to prevent rapid re-triggering. Also stops the motor.
 */
void REHYDRATION_HandleInterrupts() {
    
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
    R_CheckBumpers();

    Serial.println("[REHYDRATION] Moving backward until bumper is triggered...");

      while (BUMPER_STATE != 2){
        DRV8825_Move(&rehydrationMotor, 1, DRV8825_BACKWARD, 500); // one step at a time
        R_CheckBumpers();

    }

    Rehydration_Stop();
    Serial.println("[REHYDRATION] Back bumper triggered — motion stopped.");
}



/**
 * @brief Reads bumper interrupt flags and updates global BUMPER_STATE.
 * Now includes software debouncing to prevent false triggers.
 *
 * @return 1 = front bumper triggered, 2 = back bumper triggered, 0 = none
 */
int R_CheckBumpers()
{
  static unsigned long lastFrontTriggerTime = 0;
  static unsigned long lastBackTriggerTime = 0;
  unsigned long now = millis();
  
  if (rehydrationFrontTriggered)
  {
    // Software debouncing: ignore triggers within 50ms of last trigger
    if (now - lastFrontTriggerTime > 50) {
      rehydrationFrontTriggered = false; // Reset flag
      lastFrontTriggerTime = now;
      BUMPER_STATE = 1;
      Serial.println("[Rehydration] Front bumper triggered.");
      return 1;
    } else {
      rehydrationFrontTriggered = false; // Reset flag but ignore trigger
    }
  }
  if (rehydrationBackTriggered)
  {
    // Software debouncing: ignore triggers within 50ms of last trigger
    if (now - lastBackTriggerTime > 50) {
      rehydrationBackTriggered = false; // Reset flag
      lastBackTriggerTime = now;
      BUMPER_STATE = 2;
      Serial.println("[Rehydration] Back bumper triggered.");
      return 2;
    } else {
      rehydrationBackTriggered = false; // Reset flag but ignore trigger
    }
  }
  BUMPER_STATE = 0;
  return 0;
}

/**
 * @brief Test function to measure total syringe travel in steps
 * 
 * Moves syringe back until back bumper, then forward in 1/16th steps
 * until front bumper to count total steps in travel range.
 * 
 * @return Total number of steps between bumpers
 */
uint32_t Rehydration_CalibrationTest() {
    uint32_t stepCount = 0;
    
    // First move back until bumper
    Serial.println("[CALIBRATION] Moving to back bumper...");
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_QUARTER_STEP);
    R_CheckBumpers();
    while (BUMPER_STATE != 2) {
        DRV8825_Move(&rehydrationMotor, 1, DRV8825_BACKWARD, 500);
        R_CheckBumpers();
        yield(); // Allow WebSocket processing
    }
    
    // Switch to 1/16th stepping for precise measurement
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);
    delay(100); // Short pause between direction changes
    
    // Move forward counting steps until front bumper
    Serial.println("[CALIBRATION] Counting steps to front bumper...");
    R_CheckBumpers();
    while (BUMPER_STATE != 1) {
        DRV8825_Move(&rehydrationMotor, 1, DRV8825_FORWARD, 500);
        stepCount++;
        R_CheckBumpers();
        
        // Progress update every 1000 steps
        if (stepCount % 1000 == 0) {
            Serial.printf("[CALIBRATION] Steps so far: %lu\n", stepCount);
            yield(); // Allow WebSocket processing
        }
    }
    
    Serial.printf("[CALIBRATION] Total steps (1/16th): %lu\n", stepCount);
    Serial.printf("[CALIBRATION] Approximate full steps: %lu\n", stepCount/16);
    
    Rehydration_Stop();
    return stepCount;
}

#ifdef REHYDRATION_TEST

void setup() {
    Serial.begin(115200);
    delay(2000); // Allow USB Serial to connect
    
    Serial.println("\n=== REHYDRATION SYSTEM TEST ===");
    
    // Initialize system
    REHYDRATION_ConfigureInterrupts();
    Rehydration_InitAndDisable();
    
    // Run calibration test
    Serial.println("\nStarting calibration test...");
    uint32_t total_steps = Rehydration_CalibrationTest();
    
    Serial.println("\n=== TEST RESULTS ===");
    Serial.printf("Total travel: %lu sixteenth steps\n", total_steps);
    Serial.printf("Full steps equivalent: %lu\n", total_steps/16);
    Serial.printf("Estimated travel distance: %.2f mm\n", 
        (float)(total_steps/16) * STEP_TRAVEL_IN * 25.4);
    
    Serial.println("\nTest complete. System halted.");
    while(true) { delay(1000); } // Hold here
}

void loop() {
    // Empty - all work done in setup()
}

#endif // REHYDRATION_TEST


#ifdef REHYDRATION_TEST_2

void setup() {
    Serial.begin(115200);
    delay(2000); // Allow USB Serial to connect

    Serial.println("\n=== REHYDRATION SYSTEM TEST ===");

    // Initialize system
    REHYDRATION_ConfigureInterrupts();
    Rehydration_InitAndDisable();

    Serial.println("Type 'p' to push 1200uL, 'r' to retract 1200uL, or 'b' to back until bumper.");
    Serial.println("Ready for commands...");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        float syringeDiameter = 0.5; // 3/4 inch syringe
        switch (cmd) {
            case 'p':
            case 'P':
                Serial.println("[TEST] Pushing 1200uL...");
                Rehydration_Push(1200, syringeDiameter);
                break;
            case 'r':
            case 'R':
                Serial.println("[TEST] Retracting 1200uL...");
                Rehydration_Pull(1200, syringeDiameter);
                break;
            case 'b':
            case 'B':
                Serial.println("[TEST] Moving back until bumper...");
                Rehydration_BackUntilBumper();
                break;
            default:
                Serial.println("[TEST] Unknown command. Use 'p' (push), 'r' (retract), or 'b' (back until bumper).");
                break;
        }
    }
    delay(10);
}

#endif // REHYDRATION_TEST_2

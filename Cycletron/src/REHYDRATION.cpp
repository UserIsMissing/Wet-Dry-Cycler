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
 #include "DRV8825.h"
 #include <math.h>
 
 // === Motor Configuration ===
 DRV8825_t rehydrationMotor = {
   .step_pin = 1,     
   .dir_pin = 2,      
   .fault_pin = 42,   
   .mode0_pin = 41,   
   .mode1_pin = 40,   
   .mode2_pin = 39,   
   .enable_pin = 38   
 };
 
 // === Syringe & Motion Parameters ===
 #define STEPPER_STEPS_PER_REV 200     // Full steps per revolution of stepper motor
 #define MICROSTEPPING         16      // Microstepping mode (e.g., 1/16 step)
 #define LEADSCREW_TPI         20      // Threads per inch of leadscrew
 #define SYRINGE_DIAMETER_IN   1.0     // Syringe barrel inner diameter in inches
 
 // === Conversion Factors ===
 #define TOTAL_STEPS_PER_REV (STEPPER_STEPS_PER_REV * MICROSTEPPING)
 #define LEADSCREW_TRAVEL_IN_PER_REV (1.0 / LEADSCREW_TPI)
 #define STEP_TRAVEL_IN (LEADSCREW_TRAVEL_IN_PER_REV / TOTAL_STEPS_PER_REV)
 #define INCH3_TO_UL 16387.064  // Conversion factor from in³ to microliters
 
 /**
  * @brief Calculates how many microliters are moved per step.
  * 
  * Based on the cross-sectional area of the syringe barrel and the
  * travel per step of the leadscrew.
  *
  * @return volume in µL moved by one step
  */
 static float calculate_uL_per_step() {
   float radius_in = SYRINGE_DIAMETER_IN / 2.0;
   float step_volume_in3 = M_PI * radius_in * radius_in * STEP_TRAVEL_IN;
   float total_uL = step_volume_in3 * INCH3_TO_UL;
   return total_uL;
 }
 
 /**
  * @brief Initializes the syringe pump motor and prints calibration data.
  *
  * This function configures the DRV8825 motor with a default microstepping mode,
  * calculates the microliters-per-step constant, and displays debug info.
  */
 void Rehydration_Init() {
   DRV8825_Init(&rehydrationMotor);
   DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);
 
   float uL_per_step = calculate_uL_per_step();
   Serial.println("[REHYDRATION] Motor initialized.");
   Serial.printf("[REHYDRATION] uL per step = %.5f\n", uL_per_step);
 }
 
 /**
  * @brief Dispenses fluid by pushing the syringe plunger forward.
  *
  * Converts microliters into steps and sends a movement command.
  *
  * @param uL Volume to dispense in microliters
  */
 void Rehydration_Push(uint32_t uL) {
   DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);
 
   float uL_per_step = calculate_uL_per_step();
   uint32_t steps = (uint32_t)(uL / uL_per_step);
 
   Serial.printf("[REHYDRATION] Pushing %lu uL (%lu steps)\n", uL, steps);
   DRV8825_Move(&rehydrationMotor, steps, DRV8825_FORWARD, 500);  // Push plunger
 }
 
 /**
  * @brief Pulls fluid into the syringe (or primes it).
  *
  * Moves the motor in reverse to retract plunger and draw fluid.
  *
  * @param uL Volume to draw back in microliters
  */
 void Rehydration_Pull(uint32_t uL) {
   DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_QUARTER_STEP);  // More control
 
   float uL_per_step = calculate_uL_per_step();
   uint32_t steps = (uint32_t)(uL / uL_per_step);
 
   Serial.printf("[REHYDRATION] Retracting %lu uL (%lu steps)\n", uL, steps);
   DRV8825_Move(&rehydrationMotor, steps, DRV8825_BACKWARD, DRV8825_DEFAULT_STEP_DELAY_US);
 }
 
 /**
  * @brief Stops and disables the syringe pump motor.
  *
  * Use this after completing fluid movement to reduce heat and power draw.
  */
 void Rehydration_Stop() {
   DRV8825_Disable(&rehydrationMotor);
   Serial.println("[REHYDRATION] Motor stopped.");
 }
 
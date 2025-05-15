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
 
 // === Constants ===
 #define MOVEMENT_STEP_DELAY_US 1000  // Delay between microsteps
 
 // === Global State ===
 int BUMPER_STATE = 0;
 
 // === Motor and Sensor Config ===
 DRV8825_t movementMotor = {
   .step_pin = 6,    
   .dir_pin = 7,    
   .fault_pin = 15,    
   .mode0_pin = 16,    
   .mode1_pin = 17,    
   .mode2_pin = 18,    
   .enable_pin = 8    
 };
 
 BUMPER_t bumpers = {
   .front_bumper_pin = 3,   
   .back_bumper_pin = 10,    
   .start_button_pin = 9    
 };
 
 /**
  * @brief Applies small reverse motion to unstuck the mechanism.
  *
  * Used during initialization to nudge the system out of an uncertain
  * state before entering normal operation.
  */
 void MOVEMENT_First_Steps(int InitialSmallSteps, int UndoDirection) {
   DRV8825_Set_Step_Mode(&movementMotor, DRV8825_HALF_STEP);  // Use half-step for precision
   DRV8825_Move(&movementMotor, InitialSmallSteps, UndoDirection, MOVEMENT_STEP_DELAY_US);
   DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP);  // Return to full step mode
 }
 
 /**
  * @brief Initializes the MOVEMENT system.
  *
  * Calibrates motor direction using bumper input and prepares the system
  * for safe directional control. It attempts to find the front bumper
  * and align to it before stopping.
  */
 void MOVEMENT_Init() {
   delay(500);  // Delay for system stability
 
   DRV8825_Init(&movementMotor);  // Initialize motor driver
   CheckBumpers();                // Read initial bumper state
 
   Serial.printf("Initial BUMPER_STATE: %d\n", BUMPER_STATE);
 
   if (BUMPER_STATE == 0) {
     // No bumper pressed — back up until front is found
     MOVEMENT_First_Steps(5, DRV8825_BACKWARD);
     while (BUMPER_STATE == 0) {
       DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US);
       CheckBumpers();
     }
     DRV8825_Disable(&movementMotor);
   }
   else if (BUMPER_STATE == 1) {
     // Already at front bumper — do safety repositioning
     MOVEMENT_First_Steps(5000, DRV8825_FORWARD);
     MOVEMENT_First_Steps(1000, DRV8825_BACKWARD);
     DRV8825_Set_Step_Mode(&movementMotor, DRV8825_HALF_STEP);
     delay(2000);
     CheckBumpers();
 
     while (BUMPER_STATE != 1) {
       DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US);
       CheckBumpers();
     }
     DRV8825_Disable(&movementMotor);
   }
   else if (BUMPER_STATE == 2) {
     // At back bumper — move forward until front bumper reached
     MOVEMENT_First_Steps(50, DRV8825_BACKWARD);
     while (BUMPER_STATE != 1) {
       DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US);
       CheckBumpers();
     }
     DRV8825_Disable(&movementMotor);
   } else {
     Serial.println("[MOVEMENT] Impossible bumper state.");
   }
 
   Serial.println("[MOVEMENT] Initialization complete.");
 }
 
 /**
  * @brief Checks if the DRV8825 driver is reporting a fault.
  *
  * @return 1 if fault detected, 0 otherwise
  */
 int CheckFAULT(DRV8825_t *motor) {
   if (digitalRead(motor->fault_pin) == HIGH) {
     Serial.println("[MOVEMENT] Fault detected!");
     return 1;
   }
   return 0;
 }
 
 /**
  * @brief Reads bumper GPIOs and updates global BUMPER_STATE.
  *
  * @return 1 = front bumper, 2 = back bumper, 0 = none
  */
 int CheckBumpers() {
   if (digitalRead(bumpers.front_bumper_pin) == LOW) {
     BUMPER_STATE = 1;
     Serial.println("[MOVEMENT] Front bumper pressed.");
     return 1;
   }
   if (digitalRead(bumpers.back_bumper_pin) == LOW) {
     BUMPER_STATE = 2;
     Serial.println("[MOVEMENT] Back bumper pressed.");
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
 void MOVEMENT_Move() {
   DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP);
   CheckBumpers();
 
   if (BUMPER_STATE == 1) {
     while (BUMPER_STATE != 2) {
       DRV8825_Move(&movementMotor, 1, DRV8825_FORWARD, MOVEMENT_STEP_DELAY_US);
       CheckBumpers();
     }
     MOVEMENT_Stop();
   }
   else if (BUMPER_STATE == 2) {
     while (BUMPER_STATE != 1) {
       DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US);
       CheckBumpers();
     }
     MOVEMENT_Stop();
   }
   else {
     CheckFAULT(&movementMotor);
   }
 }
 
 /**
  * @brief Stops the motor by setting step pin LOW.
  */
 void MOVEMENT_Stop() {
   digitalWrite(movementMotor.step_pin, LOW);
   DRV8825_Disable(&movementMotor);
   Serial.println("[MOVEMENT] Motor stopped.");
 }
 
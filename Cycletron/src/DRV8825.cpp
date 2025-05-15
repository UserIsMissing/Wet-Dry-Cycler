/**
 * @file    DRV8825.cpp
 * @brief   DRV8825 Stepper Motor Driver - Arduino/ESP32 compatible
 *
 * Provides initialization and control functions for a DRV8825 motor driver
 * with configurable pin mappings. Supports direction control, step pulses,
 * microstepping, and fault checking.
 *
 * @author  Rafael Delwart
 * @date    10 Apr 2025 adapted for ESP 13 May 2025
 */

 #include <Arduino.h>
 #include "DRV8825.h"
 
//  #define DRV8825_TEST
 
 /**
  * @brief Initializes all GPIO pins used by the DRV8825 motor driver.
  *        This function must be called before any motor movement.
  */
 void DRV8825_Init(DRV8825_t *motor) {
   // Configure motor control pins as OUTPUT
   pinMode(motor->step_pin, OUTPUT);
   pinMode(motor->dir_pin, OUTPUT);
   pinMode(motor->enable_pin, OUTPUT);
   pinMode(motor->mode0_pin, OUTPUT);
   pinMode(motor->mode1_pin, OUTPUT);
   pinMode(motor->mode2_pin, OUTPUT);
 
   // Fault pin is INPUT because it receives status from the driver
   pinMode(motor->fault_pin, INPUT);
 
   // Set default values: motor off, forward direction
   digitalWrite(motor->step_pin, LOW);
   digitalWrite(motor->dir_pin, DRV8825_FORWARD);
   DRV8825_Disable(motor);  // Ensure motor starts disabled
 }
 
 /**
  * @brief Checks whether the motor driver is in a fault state.
  *        The fault pin is active-low, so HIGH means fault.
  *
  * @return true if a fault is detected, false otherwise.
  */
 bool DRV8825_Check_Fault(DRV8825_t *motor) {
   return digitalRead(motor->fault_pin) == HIGH;
 }
 
 /**
  * @brief Enables the motor driver (logic LOW = enabled).
  */
 void DRV8825_Enable(DRV8825_t *motor) {
   digitalWrite(motor->enable_pin, LOW);  // Active-low enable
 }
 
 /**
  * @brief Disables the motor driver (logic HIGH = disabled).
  */
 void DRV8825_Disable(DRV8825_t *motor) {
   digitalWrite(motor->enable_pin, HIGH);
 }
 
 /**
  * @brief Sets the direction of motor rotation.
  *        HIGH for forward, LOW for backward.
  */
 void DRV8825_Set_Direction(DRV8825_t *motor, int direction) {
   if (direction == DRV8825_FORWARD) {
     digitalWrite(motor->dir_pin, HIGH);
   } else if (direction == DRV8825_BACKWARD) {
     digitalWrite(motor->dir_pin, LOW);
   } else {
     Serial.println("[DRV8825] Invalid direction");
   }
 }
 
 /**
  * @brief Sends a single step pulse to the motor.
  *        Each HIGH-LOW cycle advances the motor one microstep.
  */
 void DRV8825_Step(DRV8825_t *motor) {
   // Drive step pin HIGH briefly
   digitalWrite(motor->step_pin, HIGH);
   delayMicroseconds(2);  // DRV8825 min pulse: 1.9 µs
 
   // Drive step pin LOW to complete the pulse
   digitalWrite(motor->step_pin, LOW);
   delayMicroseconds(2);  // DRV8825 min low time: 1.9 µs
 }
 
 /**
  * @brief Sends multiple step pulses with delays between each.
  *        Useful for basic movement without acceleration control.
  *
  * @param steps     Number of pulses to send (microsteps).
  * @param delay_us  Time between steps (controls speed).
  */
 void DRV8825_Step_N(DRV8825_t *motor, int steps, int delay_us) {
   DRV8825_Enable(motor);  // Enable motor driver
 
   for (int i = 0; i < steps; i++) {
     // Stop if fault is detected
     if (DRV8825_Check_Fault(motor)) {
       Serial.printf("[DRV8825] Fault detected at step %d\n", i);
       break;
     }
 
     DRV8825_Step(motor);         // Send one step pulse
     delayMicroseconds(delay_us); // Wait before next pulse
   }
 
   DRV8825_Disable(motor);  // Disable motor to conserve power
 }
 
 /**
  * @brief Moves the motor in a specific direction for a number of steps.
  *        Calls both direction-setting and stepping functions.
  */
 void DRV8825_Move(DRV8825_t *motor, int steps, int direction, int delay_us) {
   if (DRV8825_Check_Fault(motor)) {
     Serial.println("[DRV8825] Fault detected before move. Aborting.");
     return;
   }
 
   DRV8825_Set_Direction(motor, direction);      // Set rotation direction
   DRV8825_Step_N(motor, steps, delay_us);       // Execute movement
 }
 
 /**
  * @brief Configures the microstepping mode by setting MODE0–2 pins.
  *        Values range from 0 (full step) to 7 (1/32 step).
  *
  *        Mode pins are binary-coded: MODE2:MODE1:MODE0
  */
 void DRV8825_Set_Step_Mode(DRV8825_t *motor, int mode) {
   digitalWrite(motor->mode0_pin, mode & 0x01);         // LSB
   digitalWrite(motor->mode1_pin, (mode >> 1) & 0x01);  // Mid
   digitalWrite(motor->mode2_pin, (mode >> 2) & 0x01);  // MSB
 }
 
 


#ifdef DRV8825_TEST

#include <Arduino.h>
#include "DRV8825.h"  // Ensure this header matches your ESP32 driver

// Rehydration motor instance
DRV8825_t rehydrationMotor = {
    .step_pin = 1,     // Replace with your actual ESP32 GPIOs
    .dir_pin = 2,
    .fault_pin = 42,
    .mode0_pin = 41,
    .mode1_pin = 40,
    .mode2_pin = 39,
    .enable_pin = 38
};

void setup() {
  Serial.begin(115200);
  DRV8825_Init(&rehydrationMotor);
  DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_THIRTYSECOND_STEP);

  Serial.println("Moving forward...");
  DRV8825_Move(&rehydrationMotor, 128000, DRV8825_FORWARD, DRV8825_DEFAULT_STEP_DELAY_US);

  // Optional: Hold after move
  while (true);
}

void loop() {
  // Empty because all actions are handled in setup()
}

#endif // DRV8825_TEST
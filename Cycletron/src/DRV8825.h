/**
 * @file    DRV8825.h
 * @brief   Header for DRV8825 Stepper Motor Driver control
 *
 * Allows pin-configurable stepper motor control using the DRV8825 driver.
 * Supports step mode selection, direction, fault detection, and batch stepping.
 * Arduino/ESP32 compatible.
 *
 * Author: Rafael Delwart
 * Date:   10 Apr 2025 adapted for ESP 13 May 2025
 */

 #ifndef DRV8825_H
 #define DRV8825_H
 
 #include <Arduino.h>
 
 // === Direction Constants ===
 #define DRV8825_FORWARD  1
 #define DRV8825_BACKWARD 0
 
 // === Default Step Delay (us) ===
 #define DRV8825_DEFAULT_STEP_DELAY_US 1000
 
 // === Microstepping Modes (MODE2:MODE1:MODE0 binary format) ===
 #define DRV8825_FULL_STEP          0  // 000
 #define DRV8825_HALF_STEP          1  // 001
 #define DRV8825_QUARTER_STEP       2  // 010
 #define DRV8825_EIGHTH_STEP        3  // 011
 #define DRV8825_SIXTEENTH_STEP     4  // 100
 #define DRV8825_THIRTYSECOND_STEP  7  // 111
 
 // === DRV8825 Motor Struct ===
 
 /**
  * @struct DRV8825_t
  * @brief  Struct representing a DRV8825 motor with configurable pin mapping.
  *
  * Allows complete control over DRV8825 driver hardware, supporting per-motor
  * pin configuration for scalable multi-motor systems.
  */
 typedef struct {
   int step_pin;    ///< Pin used for step pulses
   int dir_pin;     ///< Pin used for direction control
   int fault_pin;   ///< Pin used for fault detection (active-low)
   int mode0_pin;   ///< Pin for microstepping mode bit 0
   int mode1_pin;   ///< Pin for microstepping mode bit 1
   int mode2_pin;   ///< Pin for microstepping mode bit 2
   int enable_pin;  ///< Pin used to enable/disable motor driver (active-low)
 } DRV8825_t;
 
 // === API Function Prototypes ===
 
 /**
  * Initializes the DRV8825 GPIO pins for a specific motor instance.
  * Sets all required pins to appropriate modes and disables the driver by default.
  * 
  * @param motor Pointer to DRV8825_t struct with pin configuration
  */
 void DRV8825_Init(DRV8825_t *motor);
 
 /**
  * Checks the FAULT line of the motor driver.
  * Fault is active-low; this function returns true if a fault is present.
  *
  * @param motor Pointer to DRV8825_t struct
  * @return true if fault is detected, false otherwise
  */
 bool DRV8825_Check_Fault(DRV8825_t *motor);
 
 /**
  * Enables the DRV8825 driver by setting the ENABLE pin LOW.
  * 
  * @param motor Pointer to DRV8825_t struct
  */
 void DRV8825_Enable(DRV8825_t *motor);
 
 /**
  * Disables the DRV8825 driver by setting the ENABLE pin HIGH.
  * 
  * @param motor Pointer to DRV8825_t struct
  */
 void DRV8825_Disable(DRV8825_t *motor);
 
 /**
  * Sets the motor rotation direction.
  *
  * @param motor Pointer to DRV8825_t struct
  * @param direction DRV8825_FORWARD or DRV8825_BACKWARD
  */
 void DRV8825_Set_Direction(DRV8825_t *motor, int direction);
 
 /**
  * Sends a single step pulse to the motor.
  * 
  * @param motor Pointer to DRV8825_t struct
  */
 void DRV8825_Step(DRV8825_t *motor);
 
 /**
  * Sends a specified number of step pulses to the motor, each separated by a delay.
  *
  * @param motor Pointer to DRV8825_t struct
  * @param steps Number of steps to take
  * @param delay_us Delay in microseconds between each step
  */
 void DRV8825_Step_N(DRV8825_t *motor, int steps, int delay_us);
 
 /**
  * Moves the motor a specified number of steps in a given direction.
  * Combines direction setting and batch stepping in one call.
  *
  * @param motor Pointer to DRV8825_t struct
  * @param steps Number of steps to move
  * @param direction DRV8825_FORWARD or DRV8825_BACKWARD
  * @param delay_us Delay in microseconds between steps
  */
 void DRV8825_Move(DRV8825_t *motor, int steps, int direction, int delay_us);
 
 /**
  * Sets the microstepping mode of the driver by setting MODE0–2 pins.
  *
  * @param motor Pointer to DRV8825_t struct
  * @param mode Integer from 0 to 7 representing MODE2:MODE1:MODE0 bits
  */
 void DRV8825_Set_Step_Mode(DRV8825_t *motor, int mode);
 
 #endif  // DRV8825_H
 
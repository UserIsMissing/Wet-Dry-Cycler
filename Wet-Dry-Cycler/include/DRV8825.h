/**
 * @file    DRV8825.h
 *
 * @brief   DRV8825 Stepper Motor Driver Header - Generalized for Multi-Motor Control
 *
 * This header defines the structure and functions needed to operate one or more
 * DRV8825 stepper motor drivers, using configurable pin mappings per motor instance.
 *
 * @author  Rafael Delwart
 * @date    10 Apr 2025
 */

 #ifndef DRV8825_H
 #define DRV8825_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <Board.h>
 #include <timers.h>
 #include <GPIO.h>
 
 // Direction Constants
 #define DRV8825_FORWARD  1
 #define DRV8825_BACKWARD 0
 
 // Default timing constant
 #define DRV8825_DEFAULT_STEP_DELAY_US 1000  // Default delay between steps in microseconds
 
 // Microstepping Mode Definitions
 #define DRV8825_FULL_STEP          0  // 000
 #define DRV8825_HALF_STEP          1  // 001
 #define DRV8825_QUARTER_STEP       2  // 010
 #define DRV8825_EIGHTH_STEP        3  // 011
 #define DRV8825_SIXTEENTH_STEP     4  // 100
 #define DRV8825_THIRTYSECOND_STEP  7  // 111
 
 /**
  * @struct DRV8825_t
  * @brief  Struct representing a DRV8825 motor with configurable pin mapping
  */
 typedef struct {
     int step_pin;    ///< Pin used for step pulses
     int dir_pin;     ///< Pin used for direction control
     int fault_pin;   ///< Pin used for fault detection (active-low)
     int mode0_pin;   ///< Pin for microstepping mode bit 0
     int mode1_pin;   ///< Pin for microstepping mode bit 1
     int mode2_pin;   ///< Pin for microstepping mode bit 2
     int enable_pin;  ///< Pin used to enable the motor controller (active-low)

 } DRV8825_t;
 
 /**
  * @brief Initializes DRV8825 pins for a specific motor instance
  * @param motor Pointer to DRV8825_t struct with pin configuration
  */
 void DRV8825_Init(DRV8825_t *motor);
 
 /**
  * @brief Checks the fault line of the motor
  * @param motor Pointer to DRV8825_t struct
  * @return 1 if fault is detected, 0 otherwise
  */
 int DRV8825_Check_Fault(DRV8825_t *motor);


 /**
 * @brief Enables the motor driver (active-low enable pin)
 * @param motor Pointer to DRV8825_t struct
 */
void DRV8825_Enable(DRV8825_t *motor);

/**
 * @brief Disables the motor driver (active-low enable pin)
 * @param motor Pointer to DRV8825_t struct
 */
void DRV8825_Disable(DRV8825_t *motor);
 
 /**
  * @brief Sets the motor direction (forward or backward)
  * @param motor Pointer to DRV8825_t struct
  * @param direction DRV8825_FORWARD or DRV8825_BACKWARD
  */
 void DRV8825_Set_Direction(DRV8825_t *motor, int direction);
 
 /**
  * @brief Sends a single step pulse to the motor
  * @param motor Pointer to DRV8825_t struct
  */
 void DRV8825_Step(DRV8825_t *motor);
 
 /**
  * @brief Sends a specified number of step pulses with delay
  * @param motor Pointer to DRV8825_t struct
  * @param steps Number of steps to take
  * @param delay_us Delay in microseconds between each step
  */
 void DRV8825_Step_N(DRV8825_t *motor, int steps, int delay_us);
 
 /**
  * @brief Moves the motor a number of steps in a given direction
  * @param motor Pointer to DRV8825_t struct
  * @param steps Number of steps to move
  * @param direction DRV8825_FORWARD or DRV8825_BACKWARD
  * @param delay_us Delay in microseconds between steps
  */
 void DRV8825_Move(DRV8825_t *motor, int steps, int direction, int delay_us);
 
 /**
  * @brief Sets the microstepping mode for the motor driver
  * @param motor Pointer to DRV8825_t struct
  * @param mode Value from 0 to 7 representing MODE2:MODE1:MODE0 configuration
  */
 void DRV8825_Set_Step_Mode(DRV8825_t *motor, int mode);
 
 #endif /* DRV8825_H */
/**
 * @file    DRV8825.h
 *
 * DRV8825 Stepper Motor Driver Control Module
 *
 * @author  Rafael Delwart
 * @date    10 Apr 2025
 *
 * @detail  This module provides control functions for a DRV8825 stepper 
 * motor driver. It allows initialization, direction setting, stepping,
 * enabling/disabling the motor, and setting speed using delay between pulses.
 *
 */

 #ifndef DRV8825_H
 #define DRV8825_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <Board.h>
 #include <timers.h>
 #include <GPIO.h>
 
 // PINOUTS *******************************************************
 #define DRV8825_STEP_PIN     PIN_1  // IO SHIELD: ____, STM32 PC1, PIN 36
 #define DRV8825_DIR_PIN      PIN_3  // IO SHIELD: ____, STM32 PC3, PIN 37
 #define DRV8825_FAULT_PIN  PIN_0  // IO SHIELD: ____., STM32 PC0, PIN 38
 
 #define DRV8825_MODE0_PIN    PIN_13  // MODE0 (e.g., STM32 23)
 #define DRV8825_MODE1_PIN    PIN_14  // MODE1 (e.g., STM32 25)
 #define DRV8825_MODE2_PIN    PIN_15  // MODE2 (e.g., STM32 27)
 
 // CONSTANTS *****************************************************
 #define DRV8825_FORWARD  1
 #define DRV8825_BACKWARD 0
 
 #define DRV8825_DEFAULT_STEP_DELAY_US 1000  // microseconds between steps
 
 // Microstepping mode lookup (for reference)
 #define DRV8825_FULL_STEP        0  // 000
 #define DRV8825_HALF_STEP        1  // 001
 #define DRV8825_QUARTER_STEP     2  // 010
 #define DRV8825_EIGHTH_STEP      3  // 011
 #define DRV8825_SIXTEENTH_STEP   4  // 100
 #define DRV8825_THIRTYSECOND_STEP 5 // 111


 /** DRV8825_Init()
  *
  * Initializes the DRV8825 motor driver pins. Sets the direction, step, and enable pins as outputs.
  */
 void DRV8825_Init(void);
 

 /**
 * @function DRV8825_Check_Fault(void)
 * @brief Returns TRUE if fault is active (LOW), FALSE otherwise
 */
int DRV8825_Check_Fault(void);


 /** DRV8825_Set_Direction(int direction)
  *
  * Sets the direction of the motor.
  * 
  * @param  direction  Either DRV8825_FORWARD or DRV8825_BACKWARD
  */
 void DRV8825_Set_Direction(int direction);
 
 
 /** DRV8825_Step()
  *
  * Sends a single step pulse to the motor driver.
  */
 void DRV8825_Step(void);
 
 
 /** DRV8825_Step_N(int steps, int delay_us)
  *
  * Sends N step pulses with specified delay between them.
  * 
  * @param  steps      Number of steps to take
  * @param  delay_us   Delay in microseconds between steps
  */
 void DRV8825_Step_N(int steps, int delay_us);
 
 
 /** DRV8825_Move(int steps, int direction, int delay_us)
  *
  * Moves the motor a given number of steps in the specified direction.
  * 
  * @param  steps      Number of steps
  * @param  direction  DRV8825_FORWARD or DRV8825_BACKWARD
  * @param  delay_us   Delay in microseconds between steps
  */
 void DRV8825_Move(int steps, int direction, int delay_us);


 /** DRV8825_Set_Step_Mode(int mode)
 *
 * Sets the microstepping mode by configuring MODE0, MODE1, and MODE2 pins.
 *
 * Mode table (binary encoded):
 *    MODE2:MODE1:MODE0
 *  - 000 = Full step
 *  - 001 = Half step
 *  - 010 = 1/4 step
 *  - 011 = 1/8 step
 *  - 100 = 1/16 step
 *  - 111 = 1/32 step
 *
 * @param  mode  Integer representing binary value of MODE2:MODE1:MODE0
 */
void DRV8825_Set_Step_Mode(int mode);

 
#endif  /* DRV8825_H */
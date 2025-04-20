/**
 * @file    DRV8825.c
 *
 * @brief   DRV8825 Stepper Motor Driver Control Module - Generalized for Multiple Motors
 *
 * This module provides functions to control DRV8825 stepper motor drivers
 * using customizable GPIO pin configurations per motor instance.
 *
 * @author  Rafael Delwart
 * @date    10 Apr 2025
 */

 #include <stdio.h>
 #include <stdint.h>
 #include <GPIO.h>
 #include <timers.h>
 #include "DRV8825.h"
 

 #define DRV8825_REHYDRATION_TEST
 /**
  * @function DRV8825_Init
  * @brief Initializes DRV8825 driver pins for a specific motor instance
  *
  * @param motor Pointer to DRV8825_t struct containing the pin configuration
  */
 void DRV8825_Init(DRV8825_t *motor) {
     GPIO_Init();
     TIMER_Init();
     GPIO_WritePin(motor->step_pin, LOW);
     GPIO_WritePin(motor->dir_pin, DRV8825_FORWARD);
     DRV8825_Set_Step_Mode(motor, DRV8825_FULL_STEP);
     DRV8825_Disable(motor);  // Ensure motor is not active on start up 
     if (DRV8825_Check_Fault(motor)) {
         printf("DRV8825 FAULT on INIT: Check wiring or overcurrent\n");
     }
 }
 
 /**
  * @function DRV8825_Check_Fault
  * @brief Checks if a fault condition is active for a specific motor
  *
  * @param motor Pointer to DRV8825_t struct
  * @return int TRUE (1) if fault is active, FALSE (0) otherwise
  */
 int DRV8825_Check_Fault(DRV8825_t *motor) {
     return (GPIO_ReadPin(motor->fault_pin) == 1); // Active-low
 }
 

/**
 * @function DRV8825_Enable
 * @brief Enables the motor driver by setting the enable pin LOW
 *
 * @param motor Pointer to DRV8825_t struct
 */
void DRV8825_Enable(DRV8825_t *motor) {
    GPIO_WritePin(motor->enable_pin, LOW); // Active-low enable
}

/**
 * @function DRV8825_Disable
 * @brief Disables the motor driver by setting the enable pin HIGH
 *
 * @param motor Pointer to DRV8825_t struct
 */
void DRV8825_Disable(DRV8825_t *motor) {
    GPIO_WritePin(motor->enable_pin, HIGH); // Disable output
}
 /**
  * @function DRV8825_Set_Direction
  * @brief Sets direction of stepper motor rotation
  *
  * @param motor Pointer to DRV8825_t struct
  * @param direction Either DRV8825_FORWARD or DRV8825_BACKWARD
  */
 void DRV8825_Set_Direction(DRV8825_t *motor, int direction) {
     if (direction == DRV8825_FORWARD) {
         GPIO_WritePin(motor->dir_pin, HIGH);
     } else if (direction == DRV8825_BACKWARD) {
         GPIO_WritePin(motor->dir_pin, LOW);
     } else {
         printf("DRV8825_Set_Direction ERROR: Invalid direction %d\r\n", direction);
     }
 }
 
 /**
  * @function DRV8825_Step
  * @brief Sends a single step pulse to motor driver
  *
  * @param motor Pointer to DRV8825_t struct
  */
 void DRV8825_Step(DRV8825_t *motor) {
     GPIO_WritePin(motor->step_pin, HIGH);
     uint32_t start = TIMERS_GetMicroSeconds();
     while ((TIMERS_GetMicroSeconds() - start) < 2); // 2 us HIGH
     GPIO_WritePin(motor->step_pin, LOW);
     start = TIMERS_GetMicroSeconds();
     while ((TIMERS_GetMicroSeconds() - start) < 2); // 2 us LOW
 }
 
 /**
  * @function DRV8825_Step_N
  * @brief Sends multiple step pulses with delay between them
  *
  * @param motor Pointer to DRV8825_t struct
  * @param steps Number of steps to send
  * @param delay_us Delay between steps in microseconds
  */
 void DRV8825_Step_N(DRV8825_t *motor, int steps, int delay_us) {
     DRV8825_Enable(motor);  // Ensure motor is active
     for (int i = 0; i < steps; i++) {
         if (DRV8825_Check_Fault(motor)) {
             printf("DRV8825 FAULT DETECTED: Motor stopped at step %d\n", i);
             return;
         }
         DRV8825_Step(motor);
         uint32_t start = TIMERS_GetMicroSeconds();
         while ((TIMERS_GetMicroSeconds() - start) < delay_us);
     }
     DRV8825_Disable(motor);  // Ensure motor is no longer active

 }
 
 /**
  * @function DRV8825_Move
  * @brief Moves the motor by a specified number of steps in a given direction
  *
  * @param motor Pointer to DRV8825_t struct
  * @param steps Number of steps
  * @param direction DRV8825_FORWARD or DRV8825_BACKWARD
  * @param delay_us Delay in microseconds between steps
  */
 void DRV8825_Move(DRV8825_t *motor, int steps, int direction, int delay_us) {
     if (DRV8825_Check_Fault(motor)) {
         printf("DRV8825 FAULT DETECTED BEFORE MOVE: Aborting\n");
         return;
     }
     DRV8825_Set_Direction(motor, direction);
     DRV8825_Step_N(motor, steps, delay_us);
 }
 
 /**
  * @function DRV8825_Set_Step_Mode
  * @brief Configures the microstepping mode using MODE0–2 pins
  *
  * @param motor Pointer to DRV8825_t struct
  * @param mode Microstepping mode (0–7)
  */
 void DRV8825_Set_Step_Mode(DRV8825_t *motor, int mode) {
     int mode0 = mode & 0x01;
     int mode1 = (mode >> 1) & 0x01;
     int mode2 = (mode >> 2) & 0x01;
 
     GPIO_WritePin(motor->mode0_pin, mode0);
     GPIO_WritePin(motor->mode1_pin, mode1);
     GPIO_WritePin(motor->mode2_pin, mode2);
 }
 
 #ifdef DRV8825_REHYDRATION_TEST
 int main(void) {
     BOARD_Init();
     TIMER_Init();
 
     // Rehydration configuration test for a single DRV8825 motor instance
     DRV8825_t rehydrationMotor = {
         .step_pin = PIN_C1,
         .dir_pin = PIN_C3,
         .fault_pin = PIN_C0,
         .mode0_pin = PIN_C13,
         .mode1_pin = PIN_C14,
         .mode2_pin = PIN_C15,
         .enable_pin = PIN_A15
     };
 
     DRV8825_Init(&rehydrationMotor);
 
     while (1) {
         DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_FULL_STEP);
         printf("Moving forward...\n");
         DRV8825_Move(&rehydrationMotor, 100, DRV8825_FORWARD, DRV8825_DEFAULT_STEP_DELAY_US);
         uint32_t delay_f = TIMERS_GetMilliSeconds();
         while ((TIMERS_GetMilliSeconds() - delay_f) < 2000);

     }
 }
 #endif
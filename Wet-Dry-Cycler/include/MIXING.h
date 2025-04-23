/**
 * @file    MIXING.h
 *
 * Motor control module for multiple motors using PWM
 *
 * @author  Rafael Delwart
 * @date    25 Feb 2025
 *
 * @detail  This module turns motors on and off via PWM. It allows for control
 * over individual pins or all motors at once using a set of predefined PWM pins.
 */

 #ifndef MIXING_H
 #define MIXING_H
 

 
 // PINOUTS *******************************************************
 #define NUM_MOTOR_PINS 3
 
 /** MIXING_Init()
  *
  * @brief Initializes PWM system and motor pins.
  */
 void MIXING_Init(void);
 
 /** MIXING_Motor_OnPin(PWM pin)
  *
  * @param pin - PWM struct
  * @brief Turns on motor at pin (100% duty cycle).
  */
 void MIXING_Motor_OnPin(uint8_t pin);
 
 /** MIXING_Motor_OffPin(PWM pin)
  *
  * @param pin - PWM struct
  * @brief Turns off motor at pin (0% duty cycle).
  */
 void MIXING_Motor_OffPin(uint8_t pin);
 
 /** MIXING_AllMotors_On()
  *
  * @brief Turns on all defined motors.
  */
 void MIXING_AllMotors_On(void);
 
 /** MIXING_AllMotors_Off()
  *
  * @brief Turns off all defined motors.
  */
 void MIXING_AllMotors_Off(void);
 
 #endif /* MIXING_H */
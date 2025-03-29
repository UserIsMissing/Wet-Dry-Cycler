/**
 * @file    Mixing.h
 *
 * Mixing Pad control module 
 *
 * @author  Rafael Delwart
 * @date    25 Feb 2025
 *
 * @detail  This module turns the motors on and off. It uses 
 * the PWM library to turn the mixing motor on and off when needed.
 * 
 * 
 *         
 *          
 */

#ifndef MIXING_H
#define MIXING_H

#include <stdio.h>
#include <stdint.h>
#include <Board.h>
#include <PWM.h>
#include <MIXING.h>


// PINOUTS *******************************************************
#define MIXING_CONTROL_PIN PWM_3 //IO SHIELD:56 STM: PA11 - CN10_14, GPIOA, GPIO PIN 11
//GND PIN IO SHIELD: 59


/** HEATING_Init()
 *
 * @param none
 * @return  none
 * @brief This function initializes the module for use. Initialization 
 * is done by initializing the PWM then opening and configuring the 
 * PWM pin used to turn the FET on.
 * @author Rafael Delwart, 25 Feb 2025
 */
void MIXING_Init(void);

/** MIXING_Motor_On(void)
 *
 * @param none
 * @return  none
 * @brief Turns on the mixing motor.
 * @author Rafael Delwart, 25 Feb 2025
 * 
 */
void MIXING_Motor_On(void);


/** MIXING_Motor_Off(void)
 *
 * @param none
 * @return  none
 * @brief Turns off the mixing motor.
 * @author Rafael Delwart, 25 Feb 2025
 * 
 */
void MIXING_Motor_Off(void);


#endif  /* MIXING_H */

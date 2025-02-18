/*
 * File:   pwm.h
 * Author: Adam Korycki
 *
 * Created on October 5, 2023
 */

#ifndef PWM_H
#define	PWM_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal_tim.h"

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim4;

// pwm channel struct with timer and channel attributes + bit mask for book keeping
typedef struct PWM {
    TIM_HandleTypeDef* timer;
    unsigned int channel;
    unsigned char mask;
} PWM;  

// user-level PWM channels
extern const PWM PWM_0;
extern const PWM PWM_1;
extern const PWM PWM_2;
extern const PWM PWM_3;
extern const PWM PWM_4;
extern const PWM PWM_5;

/**
 * @Function PWM_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Initializes the timer for the PWM system and set to the default frequency of 1 khz
 * @note  None.
 * @author Adam Korycki, 2023.10.05 */
char PWM_Init(void);

/**
 * @Function PWM_SetFrequency(unsigned int NewFrequency)
 * @param NewFrequency - new frequency to set. must be between 100 hz and 100 khz
 * @return SUCCESS OR ERROR
 * @brief  Changes the frequency of the PWM system.
 * @note  Behavior of PWM channels during Frequency change is undocumented
 * @author Adam Korycki, 2023.10.05 */
char PWM_SetFrequency(unsigned int NewFrequency);

/**
 * @Function PWM_GetFrequency(void)
 * @return Frequency of system in Hertz
 * @brief  gets the frequency of the PWM system.
 * @author Adam Korycki, 2023.10.05 */
unsigned int PWM_GetFrequency(void);

/**
 * @Function PWM_AddPin(PWM PWM_x)
 * @param PWM_x - PWM channel to enable and configure
 * @return SUCCESS OR ERROR
 * @brief  Adds new pin to the PWM system. If any pin is already active it errors
 * @author Adam Korycki, 2023.10.05 */
char PWM_AddPin(PWM PWM_x);

/**
 * Function  PWM_SetDutyCycle(PWM PWM_x, unsigned int Duty)
 * @param PWM_x - PWM channel to start and set duty cyle
 * @param Duty - duty cycle for the channel (0-100)
 * @return SUCCESS or ERROR
 * @remark Enables the pwm pin if not already enabled and sets the Duty Cycle for a Single Channel
 * @author Adam Korycki, 2023.10.05  */
char PWM_SetDutyCycle(PWM PWM_x, unsigned int Duty);

/**
 * Function: PWM_Start
 * @param PWM_x - PWM channel to start
 * @return SUCCESS or ERROR
 * @remark Starts pwm channel. Used to bring back up pwm channles after PWM_Stop()
 *         Must call PWM_AddPin() or PWM_SetDutyCycle() before using this function.
 * @author Adam Korycki, 2023.11.27  */
char PWM_Start(PWM PWM_x);

/**
 * Function: PWM_Stop
 * @param None
 * @return SUCCESS or ERROR
 * @remark Stops pwm channel. Use PWM_Start() to start channel again
 *         Must call PWM_AddPin() or PWM_SetDutyCycle() before using this function.
 * @author Adam Korycki, 2023.11.27  */
char PWM_Stop(PWM PWM_x);

/**
 * Function: PWM_End
 * @param None
 * @return SUCCESS or ERROR
 * @remark Disables the PWM sub-system and releases all pins.
 * @author Adam Korycki, 2023.10.05  */
char PWM_End(void);


#endif

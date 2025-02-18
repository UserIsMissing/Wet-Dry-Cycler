/*
 * File:   timers.h
 * Author: Adam Korycki
 *
 * Created on September 29, 2023
 */

#ifndef TIMERS_H
#define	TIMERS_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_rcc.h>
#include <stm32f4xx_hal_tim.h>

TIM_HandleTypeDef htim2;

/**
 * @function TIMER_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief Initializes the timer (TIM2) peripheral
 * @author Adam Korycki, 2023.09.29 */
char TIMER_Init(void);

/**
 * @function TIMERS_GetMilliSeconds(void)
 * @param None
 * @return current millisecond count
 * @brief ^
 * @author Adam Korycki, 2023.09.29 */
uint32_t TIMERS_GetMilliSeconds(void);

/**
 * @function TIMERS_GetMicroSeconds(void)
 * @param None
 * @return current microsecond count
 * @brief ^
 * @author Adam Korycki, 2023.09.29 */
uint32_t TIMERS_GetMicroSeconds(void);

/**
 * @function TIMERS_GetSystemClockFreq(void)
 * @param None
 * @return frequnecy of system clock in hz
 * @brief ^
 * @author Adam Korycki, 2023.09.29 */
uint32_t TIMERS_GetSystemClockFreq(void);

#endif

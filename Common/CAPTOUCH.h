/**
 * @file    CAPTOUCH.h
 *
 * Capacitive touch sensing module
 *
 * @author  Adam Korycki
 * @date    13 Nov 2023
 *
 * @detail  This module uses the external interrupt peripheral to detect touch
 *          inputs. The peripheral is configured to generate an interrupt every
 *          rising edge of pin PB5 (ENC_B) which means that the difference of
 *          two interrupts gives you the period of the signal. Use the timers.h
 *          library for timing operations.
 * 
 *          To alleviate setup headaches the code for setting up the peripheral
 *          and the interrupt is below.
 * 
 *          void CAPTOUCH_Init(void) {
 *               //Configure GPIO pin PB5 
 *               GPIO_InitTypeDef GPIO_InitStruct = {0};
 *               GPIO_InitStruct.Pin = GPIO_PIN_5;
 *               GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
 *               GPIO_InitStruct.Pull = GPIO_NOPULL;
 *               HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
 *
 *               // EXTI interrupt init
 *               HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
 *               HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
 * 
 *               // the rest of the function goes here
 *          }
 * 
 *          // external interrupt ISR for rising edge of pin PB5  
 *          void EXTI9_5_IRQHandler(void) {
 *              // EXTI line interrupt detected 
 *              if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
 *                  // Clear interrupt flag.
 *                  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);
 * 
 *                  // Anything that needs to happen on rising edge of PB5
 *                  // (ENC_B).
 *              }
 *          }
 *          
 */

#ifndef CAPTOUCH_H
#define CAPTOUCH_H

#include <Board.h>
#include <timers.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_tim.h>


/*  PROTOTYPES  */
/** CAPTOUCH_Init()
 *
 * This function initializes the module for use. Initialization is done by
 * opening and configuring timer 2, opening and configuring the GPIO pin and
 * setting up the interrupt.
 */
void CAPTOUCH_Init(void);

/** CAPTOUCH_IsTouched(void)
 *
 * Returns TRUE if finger is detected. Averaging of previous measurements may
 * occur within this function, however you are NOT allowed to do any I/O inside
 * this function.
 *
 * @return  (char)    [TRUE, FALSE]
 */
char CAPTOUCH_IsTouched(void);

void CAPTOUCH_Period(void);

#endif  /* CAPTOUCH_H */

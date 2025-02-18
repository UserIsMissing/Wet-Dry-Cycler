/* 
 * @file    QEI.h
 * @brief   Quadrature Encoder sensing module
 * @author  Adam Korycki
 * @date    November 9, 2023
 * @detail  This module uses the external interrupt module to interface to a quadrature encoder. 
 *          The peripheral is configured to generate interrupts on the rising and falling edges of pins PB4
 *          and PB5 (ENC_A and ENC_B on the IO shield).
 *
 *          To alleviate setup headaches the code for setting up the peripheral and the interrupt ISR/IRQ is below.
 *          QEI_IRQ() gets called when ENC_A (PB4) or ENC_B (PB5) change state. This is where you will write your 
 *          state machine.
 *
 *          void QEI_Init(void) {
 *               //Configure GPIO pins : PB4 PB5 
 *               GPIO_InitTypeDef GPIO_InitStruct = {0};
 *               GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
 *               GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
 *               GPIO_InitStruct.Pull = GPIO_NOPULL;
 *               HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
 *
 *               // EXTI interrupt init
 *               HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
 *               HAL_NVIC_EnableIRQ(EXTI4_IRQn);
 *               HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
 *               HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
 * 
 *               // the rest of the function goes here
 *          }
 * 
 *          // external interrupt ISR for pin PB5 
 *          void EXTI9_5_IRQHandler(void) {
 *              // EXTI line interrupt detected 
 *              if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
 *                  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5); // clear interrupt flag
 *                  QEI_IRQ();
 *              }
 *          }
 * 
 *          // external interrupt ISR for pin PB4
 *          void EXTI4_IRQHandler(void) {
 *              // EXTI line interrupt detected 
 *              if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET) {
 *                  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4); // clear interrupt flag
 *                  QEI_IRQ();
 *              }
 *          }
 * 
 *          void QEI_IRQ() {
 *              //state machine of your design
 *          }
 */

// #define	QEI_H
#ifndef QEI_H

/*******************************************************************************
 * INCLUDES                                                                    *
 ******************************************************************************/
#include <Board.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_tim.h>

#define ENC_A GPIO_PIN_4
#define ENC_B GPIO_PIN_5
 
/**
 * @function QEI_Init(void)
 * @param none
 * @brief  Enables the external GPIO interrupt, anything
 *         else that needs to be done to initialize the module. 
 * @return none
*/
void QEI_Init(void);

/**
 * @function QEI_GetPosition(void) 
 * @param none
 * @brief This function returns the current position of the Quadrature Encoder in degrees.      
*/
int QEI_GetPosition(void);

/**
 * @Function QEI_ResetPosition(void) 
 * @param  none
 * @return none
 * @brief  Resets the encoder such that it starts counting from 0.
*/
void QEI_ResetPosition(); 

void QEI_IRQ();

#endif	/* QEI_H */


/*
 * File:   leds.c
 * Author: Adam Korycki
 *
 * Created on November 15, 2023
 */

#include <stdio.h>
#include <stdint.h>
#include <Board.h>
#include <leds.h>
#include "stm32f4xx_hal.h"

/**
 * @function LEDS_Init(void)
 * @param None
 * @return None
 * @brief Initializes GPIO outputs for the eight leds on the IO shield
 * @author Adam Korycki, 2023.11.15 */
void LEDS_Init(void) {
    //enable GPIO clocks for ports C and B
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    //init GOIO output pins for leds
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    set_leds(0x0); // reset leds

}

/**
 * @function set_leds(uint8_t leds)
 * @param leds - byte where each bit represents the state of the leds (lsb -> LED1)
 * @return None
 * @brief sets leds according to byte parameter. Must call LEDS_Init() before.
 * @author Adam Korycki, 2023.11.15 */
void set_leds(uint8_t leds) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, (leds) & 0x1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, (leds >> 1) & 0x1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, (leds >> 2) & 0x1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, (leds >> 3) & 0x1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (leds >> 4) & 0x1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (leds >> 5) & 0x1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, (leds >> 6) & 0x1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, (leds >> 7) & 0x1);
}

//#define LEDS_TEST
#ifdef LEDS_TEST // LED TEST HARNESS
// SUCCESS - leds on IO shield are shown to be counting up in binary with D1 as the lsb. Counting should roll over

#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <leds.h>


int main(void) {
    BOARD_Init();
    LEDS_Init();

    for (uint8_t i = 0; i < 256; i++) {
        set_leds(i);
        HAL_Delay(200);
    }
    while (TRUE);
}
#endif
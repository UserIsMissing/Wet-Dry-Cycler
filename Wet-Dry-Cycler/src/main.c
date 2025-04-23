#include <stdio.h>
#include <stdlib.h>
#include "Board.h"
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>
#include <PWM.h>
#include <buttons.h>
#include <GPIO.h>
#include <stm32f4xx_hal.h>
// #include <stm32f4xx_it.h>

#include <HEATING.h>
#include <MIXING.h>
#include <DRV8825.h>
#include <REHYDRATION.h>
#include <MOVEMENT.h>
#include <main.h>

// ***************************************************************
/**
 * @function HAL_GPIO_EXTI_Callback
 * @author Cole Schreiner
 * @date   22 Apr 2025
 * @brief  External interrupt callback for handling bumper triggers
 * @details This function is called when any of the bumpers (PA5, PA6, PB8) are triggered.
 *          It is used for triggering removal of the sample upon external user input
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
    // case GPIO_PIN_5: // PA5 bumper
    case PIN_A5:          // PA5 bumper
        BUMPER_STATE = 1; // Set bumper state to indicate left bumper hit
        printf("Left bumper hit!\r\n");
        MOVEMENT_Stop(); // Stop movement motor
        break;
    // case GPIO_PIN_6: // PA6 bumper
    case PIN_A6:          // PA6 bumper
        BUMPER_STATE = 2; // Set bumper state to indicate right bumper hit
        printf("Right bumper hit!\r\n");
        MOVEMENT_Stop(); // Stop movement motor
        break;
    // case GPIO_PIN_8: // PB8 Start Movement button
    case PIN_B8:          // PB8 Start Movement button
        BUMPER_STATE = 3; // Set bumper state to indicate rear bumper hit
        printf("Rear bumper hit!\r\n");
        MOVEMENT_Move(); // Start movement motor
        break;
    }
default:
    // BUMPER_STATE = 0; // Reset bumper state if no bumper hit
    printf("Unknown GPIO interrupt: %d\r\n", GPIO_Pin);
    break;
}

/**
 *  @function main
 *  @author Cole Schreiner
 *  @date   22 Apr 2025
 *  @brief  Main state machine for running the system
 *  @details This function initializes all the modules and enters an infinite loop where it will
 *  run the system.
 *  @note Program flow:
 *      1. Initialize the board and all modules.
 *      2. Upon first use, system will rehydrate, mix, and heat the sample.
 *      3. Movement will only be called upon user input (button press).
 */
#define TESTING_MAIN
#ifdef TESTING_MAIN
int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();
    PWM_Init();
    GPIO_Init();

    // Set priorities and enable interrupts
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0); // PA5, PA6, PB8 share EXTI5-9
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    // MOVEMENT_Init();
    // HEATING_Init();
    // MIXING_Init();
    // REHYDRATION_Init();

    while (1)
    {
    }
}
#endif // TESTING_MAIN

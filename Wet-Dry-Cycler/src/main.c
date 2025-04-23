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
    default:
        // BUMPER_STATE = 0; // Reset bumper state if no bumper hit
        printf("Unknown GPIO interrupt: %d\r\n", GPIO_Pin);
        break;
    }
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
typedef enum {
    STATE_MIXING,
    STATE_HEATING,
    STATE_MOVING,
    STATE_DONE
} SystemState;

int main(void) {
    // Initialize hardware and modules
    BOARD_Init();
    HEATING_Init();
    MIXING_Init();
    MOVEMENT_Init();

    SystemState state = STATE_HEATING;
    const int targetTemp = 50;

    while (1) {
        switch (state) {

            case STATE_HEATING:
                printf("[STATE] HEATING: Target = %d°C\r\n", targetTemp);
                HEATING_Set_Temp(targetTemp);

                float currentTemp = HEATING_Measure_Temp_Avg();
                printf("Current Temp: %.2f°C\r\n", currentTemp);

                if (currentTemp >= targetTemp - 0.5) {
                    printf("Heating complete.\r\n");
                    HAL_Delay(500);
                    state = STATE_MIXING;
                }
                break;

            case STATE_MIXING:
                printf("[STATE] MIXING: Motors ON\r\n");
                MIXING_AllMotors_On();
                HAL_Delay(5000);
                MIXING_AllMotors_Off();
                printf("Mixing complete.\r\n");
                state = STATE_MOVING;
                break;

            case STATE_MOVING:
                printf("[STATE] MOVING: Starting motor movement\r\n");
                MOVEMENT_Move(); // Assumes internal step target
                printf("Movement complete.\r\n");
                state = STATE_DONE;
                break;

            case STATE_DONE:
                printf("[STATE] DONE: Process complete. System halting.\r\n");
                while (1); // Halt
                break;
        }

        HAL_Delay(100);
    }
}
#endif // TESTING_MAIN

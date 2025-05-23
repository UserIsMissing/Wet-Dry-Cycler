#include "main.h"

#define WATER_INTERVAL 10000   // 1 second interval for water level check
#define MIXING_INTERVAL 5000   // 5 seconds interval for mixing
#define HEATING_INTERVAL 10000 // 10 seconds interval for heating

static uint8_t toggle_movement_flag;
static int prevState;

typedef enum
{
    STATE_START,
    STATE_REHYDRATING,
    STATE_HEATING,
    STATE_MIXING,
    STATE_MOVING,
    STATE_MOVEMENT_WAITING, // Waiting for movement to complete
    STATE_DONE
} SystemState;

static SystemState state = STATE_START; // Initial state

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
    /*
        switch (GPIO_Pin)
        {
        case GPIO_PIN_8:          // PB8 Start Movement button
                                  // case PIN_B8:                   // PB8 Start Movement button
            state = STATE_MOVING; // Change state to moving
            // BUMPER_STATE = 3;          // Set bumper state to indicate rear bumper hit
            toggle_movement_flag ^= 1; // Toggle movement flag
            // printf("START BUTTON hit!\r\n");
            break;
        default:
            BUMPER_STATE = 0; // Reset bumper state if no bumper hit
            break;
        }
         */
    if (GPIO_Pin == GPIO_PIN_8)
    { // Start button
        HAL_Delay(300); // Debounce delay
        printf("Start button pressed!\n");
        state = STATE_MOVING; // Change state to moving
        toggle_movement_flag ^= 1;
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
// #define TESTING_MAIN
#ifdef TESTING_MAIN
int main(void)
{
    // Initialize hardware and modules
    BOARD_Init();
    GPIO_Init();
    HAL_Delay(500); // Wait for 500ms to ensure everything is stable
    // Enable EXTI8 interrupt for PB8
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0); // PB8 uses EXTI9_5_IRQn
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    HEATING_Init();
    MIXING_Init();
    MOVEMENT_Init();

    uint32_t RecentTime = 0;
    toggle_movement_flag = 0; // Initialize movement flag

    const int targetTemp = 40;

    printf("PB8 state: %d\n", HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8));

    while (1)
    {
        switch (state)
        {
        case STATE_START:
            RecentTime = TIMERS_GetMilliSeconds();
            printf("[STATE] START: Initializing system\r\n");
            prevState = state;         // Store previous state
            state = STATE_REHYDRATING; // Move to rehydration state
            printf("[STATE] REHYDRATING: Pushing fluid\r\n");
            break;

        case STATE_REHYDRATING:
            Rehydration_Push(1000); // Push 1000 uL of fluid
            HAL_Delay(2000);        // Wait for rehydration to complete
            RecentTime = TIMERS_GetMilliSeconds();
            prevState = state;    // Store previous state
            state = STATE_MIXING; // Move to heating state
            printf("[STATE] MIXING: Motors ON\r\n");
            break;

        case STATE_MIXING:
            MIXING_AllMotors_On();
            if ((TIMERS_GetMilliSeconds() - RecentTime) > MIXING_INTERVAL)
            {
                MIXING_AllMotors_Off();
                printf("Mixing motors OFF\r\n");
                RecentTime = TIMERS_GetMilliSeconds();
                prevState = state; // Store previous state
                state = STATE_HEATING;
                printf("[STATE] HEATING: Target = %d°C\r\n", targetTemp);
            }
            break;

        case STATE_HEATING:
            HEATING_Set_Temp(targetTemp);

            float currentTemp = HEATING_Measure_Temp_Avg();
            // printf("Current Temp: %.2f°C\r\n", currentTemp);

            if (currentTemp >= targetTemp - 0.5)
            {
                printf("Heating complete.\r\n");
                HAL_Delay(500);
                RecentTime = TIMERS_GetMilliSeconds();
                prevState = state; // Store previous state
                state = STATE_REHYDRATING;
            }
            if ((TIMERS_GetMilliSeconds() - RecentTime) > HEATING_INTERVAL)
            {
                RecentTime = TIMERS_GetMilliSeconds();
                prevState = state; // Store previous state
                state = STATE_REHYDRATING;
            }
            break;

        case STATE_MOVING:
            printf("[STATE] MOVING: Starting motor movement\r\n");
            MIXING_AllMotors_Off(); // Ensure all motors are off before moving
            HEATING_Set_Temp(0);    // Turn off heating pad
            MOVEMENT_Move();
            printf("Movement complete.\r\n");
            // if (toggle_movement_flag)
            // {
            // RecentTime = TIMERS_GetMilliSeconds();
            // prevState = state;              // Store previous state
            // MOVEMENT_Move(); // Move back to starting position
            state = STATE_MOVEMENT_WAITING; // Move to done state
            // }
            break;

        case STATE_MOVEMENT_WAITING:
            if (!toggle_movement_flag)
            {
                // RecentTime = TIMERS_GetMilliSeconds();
                prevState = state;     // Store previous state
                state = STATE_HEATING; // Move to done state
            }
            break;

        case STATE_DONE:
            printf("[STATE] DONE: Process complete. System halting.\r\n");
            while (1)
                ; // Halt
            break;
        }
        HAL_Delay(100);
    }
}
#endif // TESTING_MAIN

#ifdef TESTING_MOVEMENT
int main(void)
{
    BOARD_Init();
    TIMER_Init();
    GPIO_Init();
    HAL_Delay(5000); // Wait for 5 seconds

    // Set priorities and enable interrupts
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0); // PA5, PA6, PB8 share EXTI5-9
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    uint32_t RecentTime = 0;
    toggle_movement_flag = 0; // Initialize movement flag

    printf("MOVEMENT module initializing...\n");
    MOVEMENT_Init();
    printf("MOVEMENT init Complete\n");

    while (1)
    {
        switch (state)
        {
        case STATE_START:
            printf("[STATE] START: Initializing system\r\n");
            break;
        case STATE_MOVING:
            printf("[STATE] MOVING: Starting motor movement\r\n");
            MOVEMENT_Move();
            printf("Movement complete.\r\n");
            // if (toggle_movement_flag)
            // {
            // RecentTime = TIMERS_GetMilliSeconds();
            // prevState = state;              // Store previous state
            // MOVEMENT_Move(); // Move back to starting position
            state = STATE_MOVEMENT_WAITING; // Move to done state
            // }
            break;

        case STATE_MOVEMENT_WAITING:
            if (!toggle_movement_flag)
            {
                // RecentTime = TIMERS_GetMilliSeconds();
                prevState = state; // Store previous state
                state = STATE_START;
            }
            break;
        }
    }
}
#endif // TESTING_MOVEMENT
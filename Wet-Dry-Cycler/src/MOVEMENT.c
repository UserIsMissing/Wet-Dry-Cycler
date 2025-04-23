/**
 * @file    MOVEMENT.c
 *
 * @brief   MOVEMENT Module for controlling stepper motors in the movement subsystem
 *
 * This module provides functions to control the movement of stepper motors for the movement
 * subsystem. Important for moveing between heating, mixing, etc to extraction zone.
 *
 * @author  Cole Schreiner
 *
 * @date    14 Apr 2025
 *
 */

#include <main.h>

#define MOVEMENT_STEP_DELAY_US 1000 // Delay between steps in microseconds

int BUMPER_STATE = 0;

DRV8825_t movementMotor = {
    .step_pin = PIN_B4,
    .dir_pin = PIN_B5,
    .fault_pin = PIN_A7,
    .mode0_pin = PIN_A1,
    .mode1_pin = PIN_A4,
    .mode2_pin = PIN_B0,
    .enable_pin = PIN_C2 // added an enable pin to disable the motor when not in use (Rafael)
};

BUMPER_t bumpers = {
    .front_bumper_pin = PIN_A5,
    .back_bumper_pin = PIN_A6,
    .start_button_pin = PIN_B8};

// Undo Direction because the motor need to move the opposite direction a tiny bit before aligning
// This function helps start the motor for the first time using smaller steps to avoid getting stuck
void MOVEMENT_First_Steps(int InitialSmallSteps, int UndoDirection)
{
    DRV8825_Set_Step_Mode(&movementMotor, DRV8825_HALF_STEP);                               // Set microstepping mode
    DRV8825_Move(&movementMotor, InitialSmallSteps, UndoDirection, MOVEMENT_STEP_DELAY_US); // Move forward slightly
    DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP);                               // Set microstepping mode
}
/**
 * @function MOVEMENT_Init
 * @brief   Initializes the MOVEMENT module
 * @param   None
 * @return  None
 * @details This function initializes the MOVEMENT module by initializing the GPIO pins and the timer.
 * Moves the motor forward slightly then backwards to check for bumpers starting position.
 */
void MOVEMENT_Init(void)
{
    HAL_Delay(2000);              // Wait for 2 seconds
    DRV8825_Init(&movementMotor); // Initialize the motor driver

    printf("BUMPER_STATE: %d\n", BUMPER_STATE);
    CheckBumpers(); // Initialize the bumpers
    if (BUMPER_STATE == 0)
    {
        printf("BUMPER_STATE: %d\n", BUMPER_STATE);
        MOVEMENT_First_Steps(5, DRV8825_BACKWARD);
        while (BUMPER_STATE == 0)
        {
            DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US); // Move forward slightly
            CheckBumpers();
            if (BUMPER_STATE == 1)
            {
                DRV8825_Disable(&movementMotor);
                return;
            }
        }
        // // MOVEMENT_Stop();
        // DRV8825_Disable(&movementMotor);
        // return;
    }
    else if (BUMPER_STATE == 1)
    {
        printf("BUMPER_STATE: %d\n", BUMPER_STATE);
        MOVEMENT_First_Steps(50, DRV8825_FORWARD); // Move forward slightly
        while (BUMPER_STATE != 1)
        {
            DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US); // Move forward slightly
            CheckBumpers();
            if (BUMPER_STATE == 1)
            {
                DRV8825_Disable(&movementMotor);
                return;
            }
        }
    }
    else if (BUMPER_STATE == 2)
    {
        printf("BUMPER_STATE: %d\n", BUMPER_STATE);
        MOVEMENT_First_Steps(50, DRV8825_BACKWARD); // Move backward slightly
        while (BUMPER_STATE != 1)
        {
            DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US); // Move forward slightly
            CheckBumpers();
            if (BUMPER_STATE == 1)
            {
                DRV8825_Disable(&movementMotor);
                return;
            }
        }
    }
    else
    {
        printf("BUMPER_STATE: IMPOSSIBLE POSITION");
    }
    printf("MOVEMENT module initialized.\n");
    return;
}

/**
 * @function CheckFAULT
 * @brief   Checks the fault line of the motor
 * @param   movementMotor Pointer to DRV8825_t struct
 * @return  int 1 if fault is detected, 0 otherwise
 * @details This function checks the fault line of the motor to determine if it is in a fault state.
 */
int CheckFAULT(DRV8825_t *movementMotor)
{
    // Check if the motor is in a fault state
    if (GPIO_ReadPin(movementMotor->fault_pin) == 1)
    {
        printf("Motor fault detected!\n");
        return 1; // Fault detected
    }
    return 0; // No fault
}

/**
 * @function CheckBumpers
 * @brief   Checks the bumpers to determine if the motor should stop
 * @param   bumpers Pointer to BUMPER_t struct
 * @return  int 1 if front bumper pressed, 2 if back bumper pressed, 0 if no bumper pressed
 * @details This function checks the bumpers to determine if the motor should stop.
 */
int CheckBumpers(void /* BUMPER_t *bumpers */)
{
    // Check if the bumpers are pressed
    if (GPIO_ReadPin(bumpers.front_bumper_pin) == 0)
    {
        printf("Front bumper pressed!\n");
        BUMPER_STATE = 1;
        return 1; // Bumper pressed
    }
    if (GPIO_ReadPin(bumpers.back_bumper_pin) == 0)
    {
        printf("Back bumper pressed!\n");
        BUMPER_STATE = 2;
        return 2; // Bumper pressed
    }
    if (GPIO_ReadPin(bumpers.start_button_pin) == 1)
    {
        printf("Start button pressed!\n");
        // BUMPER_STATE = 3;
        return 3; // Start button pressed
    }
    else
    {
        BUMPER_STATE = 0;
        return 0; // No bumper pressed
    }
}

/**
 * @function MOVEMENT_Move
 * @brief   Moves the stepper motor for set ammount of time
 *
 * @param   movementMotor Pointer to the motor to be moved
//  * @param   Direction Direction to move the motor (1 for forward, -1 for backward)
//  * @param   steps Number of steps to move the motor
 * @details This function moves the motor forward for a set amount of time.
 *          It checks for faults and stops the motor if a fault is detected.
 *          It also checks the bumpers to determine if the motor should stop.
 *          The function uses the DRV8825 driver to control the motor.
 */
void MOVEMENT_Move(void)
{
    // Move the motor forward for a set amount of time
    DRV8825_Set_Step_Mode(&movementMotor, DRV8825_FULL_STEP);
    CheckBumpers();
    printf("BUMPER_STATE: %d\n", BUMPER_STATE);
    if (BUMPER_STATE == 1)
    {
        while (BUMPER_STATE != 2)
        {
            DRV8825_Move(&movementMotor, 1, DRV8825_FORWARD, MOVEMENT_STEP_DELAY_US); // Move forward slightly
            CheckBumpers();
            if (BUMPER_STATE == 2)
            {
                MOVEMENT_Stop();
                return;
            }
        }
        MOVEMENT_Stop();
        return;
    }
    if (BUMPER_STATE == 2)
    {
        while (BUMPER_STATE != 1)
        {
            DRV8825_Move(&movementMotor, 1, DRV8825_BACKWARD, MOVEMENT_STEP_DELAY_US); // Move forward slightly
            CheckBumpers();
            if (BUMPER_STATE == 1)
            {
                MOVEMENT_Stop();
                return;
            }
        }
        MOVEMENT_Stop();
        return;
    }
    else
    {
        CheckFAULT(&movementMotor);
    }
    return;
}

/**
 * @function MOVEMENT_Stop
 * @brief   Stops the stepper motor
 *
 * @param   movementMotor Pointer to the motor to be stopped
 */
void MOVEMENT_Stop(void)
{
    // Stop the motor by setting the step pin to low
    GPIO_WritePin(movementMotor.step_pin, LOW);
    printf("Motor stopped.\n");
}

// #define TESTING_MOVEMENT
#ifdef TESTING_MOVEMENT
int main(void)
{
    BOARD_Init();
    TIMER_Init();
    GPIO_Init();
    // Set priorities and enable interrupts
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0); // PA5, PA6, PB8 share EXTI5-9
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    DRV8825_Init(&movementMotor);
    printf("MOVEMENT module initializing...\n");
    MOVEMENT_Init();
    printf("MOVEMENT init Complete\n");
    HAL_Delay(5000); // Wait for 5 seconds

    while (1)
    {
        CheckBumpers();
        if (GPIO_ReadPin(bumpers.start_button_pin) == 1) // Extraction button pressed
        {
            printf("BUMPER_STATE: %d\n", BUMPER_STATE);
            HAL_Delay(5000); // Wait for 5 seconds
            printf("STARTING MOVEMENT TEST\n");
            MOVEMENT_Move();
        }
    }
}
#endif // TESTING_MOVEMENT
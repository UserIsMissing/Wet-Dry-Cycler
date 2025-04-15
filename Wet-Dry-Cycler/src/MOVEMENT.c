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

#include <stdio.h>
#include <stdint.h>
#include <Board.h>
#include <GPIO.h>
#include <timers.h>
#include <DRV8825.h>
#include <MOVEMENT.h>
#include <I2C.h>
#include <buttons.h>
#include <PWM.h>


DRV8825_t movementMotor = {
    .step_pin = PIN_1,
    .dir_pin = PIN_3,
    .fault_pin = PIN_0,
    .mode0_pin = PIN_13,
    .mode1_pin = PIN_14,
    .mode2_pin = PIN_15
};

void MOVEMENT_Init(void)
{
    // Initialize the GPIO pins for the stepper motor
    GPIO_Init();
    TIMER_Init();

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
 * @param   movementMotor Pointer to DRV8825_t struct
 * @return  int 1 if front bumper pressed, 2 if back bumper pressed, 0 if no bumper pressed
 * @details This function checks the bumpers to determine if the motor should stop.
 */
int CheckBumpers(DRV8825_t *movementMotor)
{
    // Check if the bumpers are pressed
    if (GPIO_ReadPin(movementMotor->front_bumper_pin) == 1)
    {
        printf("Front bumper pressed!\n");
        return 1; // Bumper pressed
    }
    if (GPIO_ReadPin(movementMotor->back_bumper_pin) == 1)
    {
        printf("Back bumper pressed!\n");
        return 2; // Bumper pressed
    }
    return 0; // No bumper pressed
}

/**
 * @function MOVEMENT_Forward
 * @brief   Moves the stepper motor for set ammount of time
 *
 * @param   movementMotor Pointer to the motor to be moved
 */
void MOVEMENT_Forward(DRV8825_t *movementMotor)
{
    // Move the motor forward for a set amount of time
    DRV8825_Move(movementMotor, steps, DRV8825_FORWARD, 1000);
    // Conditional if - (condition) ? (true) : (false);
    // (CheckFAULT(movementMotor)) ? return :;
    if (CheckFAULT(movementMotor))
    {
        printf("Motor fault detected!\n");
        return; // Exit if fault is detected
    }
    else
    {
        printf("Moving forward...\n");
    }
}

/**
 * @function MOVEMENT_Backward
 * @brief   Moves the stepper motor for set ammount of time
 *
 * @param   movementMotor Pointer to the motor to be moved
 */
void MOVEMENT_Backward(DRV8825_t *movementMotor)
{
    // Move the motor backward for a set amount of time
    DRV8825_Move(movementMotor, steps, DRV8825_BACKWARD, 1000);
    // Conditional if - (condition) ? (true) : (false);
    // (CheckFAULT(movementMotor)) ? return :;
    if (CheckFAULT(movementMotor))
    {
        printf("Motor fault detected!\n");
        return; // Exit if fault is detected
    }
    else
    {
        printf("Moving backward...\n");
    }
}

/**
 * @function MOVEMENT_Stop
 * @brief   Stops the stepper motor
 *
 * @param   movementMotor Pointer to the motor to be stopped
 */
void MOVEMENT_Stop(DRV8825_t *movementMotor)
{
    // Stop the motor by setting the step pin to low
    GPIO_WritePin(movementMotor->step_pin, LOW);
    printf("Motor stopped.\n");
}



#define TESTING_MOVEMENT
#ifdef TESTING_MOVEMENT
int main(void)
{
    BOARD_Init();
    TIMER_Init();
    MOVEMENT_Init();


    DRV8825_Init(&movementMotor);

    while (1)
    {
        DRV8825_Set_Step_Mode(&movementMotor, DRV8825_THIRTYSECOND_STEP);
        printf("Moving forward...\n");
        DRV8825_Move(&movementMotor, 1000, DRV8825_FORWARD, DRV8825_DEFAULT_STEP_DELAY_US);
        uint32_t delay_f = TIMERS_GetMicroSeconds();
        while ((TIMERS_GetMilliSeconds() - delay_f) < 2000)
            ;

        printf("Moving backward...\n");
        DRV8825_Move(&movementMotor, 200, DRV8825_BACKWARD, DRV8825_DEFAULT_STEP_DELAY_US);
        uint32_t delay_b = TIMERS_GetMicroSeconds();
        while ((TIMERS_GetMilliSeconds() - delay_b) < 2000)
            ;
    }
}
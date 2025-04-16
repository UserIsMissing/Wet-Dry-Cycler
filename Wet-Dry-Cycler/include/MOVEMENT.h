/**
 * @file    MOVEMENT.h
 *
 * @brief   MOVEMENT Module for controlling stepper motors in the movement subsystem
 *
 * @author  Cole Schreiner
 *
 * @date    14 Apr 2025
 */

#include <stdio.h>
#include <stdint.h>
#include <Board.h>
#include <GPIO.h>
#include <timers.h>
#include <DRV8825.h>

extern int BUMPER_STATE; // 0 = no bumper pressed, 1 = front bumper pressed, 2 = back bumper pressed

// typedef struct {
//     int step_pin;    ///< Pin used for step pulses
//     int dir_pin;     ///< Pin used for direction control
//     int fault_pin;   ///< Pin used for fault detection (active-low)
//     int mode0_pin;   ///< Pin for microstepping mode bit 0
//     int mode1_pin;   ///< Pin for microstepping mode bit 1
//     int mode2_pin;   ///< Pin for microstepping mode bit 2
// } DRV8825_t;

typedef struct {
    int front_bumper_pin; ///< Pin for front bumper switch
    int back_bumper_pin;  ///< Pin for back bumper switch
} BUMPER_t;

/** 
 * @function MOVEMENT_Init
 * @brief   Initializes the MOVEMENT module
 * @param   None
 * @return  None
 * @details This function initializes the MOVEMENT module by initializing the GPIO pins and the timer.
 */
void MOVEMENT_Init(void);

/**
 * @function CheckFAULT
 * @brief   Checks the fault line of the motor
 * @param   movementMotor Pointer to DRV8825_t struct
 * @return  int 1 if fault is detected, 0 otherwise
 * @details This function checks the fault line of the motor to determine if a fault is detected.
 */
int CheckFAULT(DRV8825_t *motor);

/**
 * @function CheckBumpers
 * @brief   Checks the bumpers to determine if the motor should stop
 * @param   bumpers Pointer to BUMPER_t struct
 * @return  int 1 if front bumper pressed, 2 if back bumper pressed, 0 if no bumper pressed
 * @details This function checks the bumpers to determine if the motor should stop.
 */
int CheckBumpers(BUMPER_t *bumpers);

/**
 * @function MOVEMENT_Move
 * @brief   Moves the stepper motor for a set amount of time
 * @param   movementMotor Pointer to the motor to be moved
 * @param   Direction Direction to move the motor (1 for forward, -1 for backward)
 * @param   steps Number of steps to move the motor
 * @details This function moves the motor forward for a set amount of time.
 *          It checks for faults and stops the motor if a fault is detected.
 *          It also checks the bumpers to determine if the motor should stop.
 *          The function uses the DRV8825 driver to control the motor.
 */
void MOVEMENT_Move(DRV8825_t *motor, int steps, int Direction);
// void MOVEMENT_Backward(DRV8825_t *motor);

/**
 * @function MOVEMENT_Stop
 * @brief   Stops the stepper motor
 * @param   movementMotor Pointer to the motor to be stopped
 * @details This function stops the stepper motor by setting the step pin to low.
 */
void MOVEMENT_Stop(DRV8825_t *motor);

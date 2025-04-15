/**
 * @file    MOVEMENT.h
 *
 * @brief   MOVEMENT Module for controlling stepper motors in the movement subsystem
 *
 * @author  Cole Schreiner
 *
 * @date    14 Apr 2025
 */


void MOVEMENT_Init(void);
void CheckFAULT(DRV8825_t *motor);
void MOVEMENT_Forward(DRV8825_t *motor);
void MOVEMENT_Backward(DRV8825_t *motor);
void MOVEMENT_Stop(DRV8825_t *motor);

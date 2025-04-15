/**
 * @file    MOVEMENT.h
 *
 * @brief   MOVEMENT Module for controlling stepper motors in the movement subsystem
 *
 * @author  Cole Schreiner
 *
 * @date    14 Apr 2025
 */

typedef struct {
    int step_pin;    ///< Pin used for step pulses
    int dir_pin;     ///< Pin used for direction control
    int fault_pin;   ///< Pin used for fault detection (active-low)
    int mode0_pin;   ///< Pin for microstepping mode bit 0
    int mode1_pin;   ///< Pin for microstepping mode bit 1
    int mode2_pin;   ///< Pin for microstepping mode bit 2
} MOVEMENT_t;

typedef struct {
    int front_bumper_pin; ///< Pin for front bumper switch
    int back_bumper_pin;  ///< Pin for back bumper switch
} BUMPER_t;

void MOVEMENT_Init(void);
void CheckFAULT(MOVEMENT_t *motor);
int CheckBumpers(MOVEMENT_t *motor);
void MOVEMENT_Forward(MOVEMENT_t *motor);
void MOVEMENT_Backward(MOVEMENT_t *motor);
void MOVEMENT_Stop(MOVEMENT_t *motor);

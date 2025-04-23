/**
 * @file    Rehydration.c
 *
 * @brief   Controls a stepper motor for precise fluid rehydration using DRV8825 driver
 *
 * This module provides initialization and volume-based motion control for a syringe pump
 * driven by a DRV8825 stepper motor. Designed for rehydration of RNA in wet-dry cycling.
 *
 * @author  Rafael Delwart
 * @date    20 Apr 2025
 */

 #include <stdio.h>
 #include <stdint.h>
 #include <DRV8825.h>
 #include <timers.h>
 #include <GPIO.h>
 #include "Rehydration.h"
 #include <Board.h>   
 #include <math.h>
 
 // Global motor configuration for syringe pump
 DRV8825_t rehydrationMotor = {
     .step_pin = PIN_C1,
     .dir_pin = PIN_C3,
     .fault_pin = PIN_C0,
     .mode0_pin = PIN_C10,
     .mode1_pin = PIN_C11,
     .mode2_pin = PIN_C12,
     .enable_pin = PIN_A15
 };
 
// Rehydration Parameters (modifiable)
#define STEPPER_STEPS_PER_REV 200     // Full steps per revolution
#define MICROSTEPPING         16      // Microstepping mode (e.g. 1/32)
#define LEADSCREW_TPI         20      // Threads per inch (TPI)
#define SYRINGE_DIAMETER_IN   1    // Syringe barrel inner diameter in inches

// Derived constants
#define TOTAL_STEPS_PER_REV (STEPPER_STEPS_PER_REV * MICROSTEPPING)
#define LEADSCREW_TRAVEL_IN_PER_REV (1.0 / LEADSCREW_TPI)
#define STEP_TRAVEL_IN (LEADSCREW_TRAVEL_IN_PER_REV / TOTAL_STEPS_PER_REV)
#define INCH3_TO_UL 16387.064
#define NUM_OUTPUT_PORTS 12

static float calculate_uL_per_step(void) {
    float radius_in = SYRINGE_DIAMETER_IN / 2.0;
    float step_volume_in3 = M_PI * radius_in * radius_in * STEP_TRAVEL_IN;
    float total_uL = step_volume_in3 * INCH3_TO_UL;
    return total_uL;
}
/**
 * @function Rehydration_Init
 * @brief    Initializes the syringe pump motor for rehydration
 */
void Rehydration_Init(void) {
    DRV8825_Init(&rehydrationMotor);
    DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);
    printf("Rehydration motor initialized.\n");
    printf("uL per step = %.5f\n", calculate_uL_per_step());
}

/**
  * @function Rehydration_Push
  * @brief    Pushes liquid by rotating motor forward for calculated steps
  * @param    uL Volume to dispense in microliters
  */
 void Rehydration_Push(uint32_t uL) {
     DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_SIXTEENTH_STEP);
     float uL_per_step = calculate_uL_per_step();
     uint32_t steps = (uint32_t)(uL / uL_per_step);
     printf("Pushing %lu uL (%lu steps)\n", uL, steps);
     DRV8825_Move(&rehydrationMotor, steps, DRV8825_FORWARD, 500);
 }
 
 /**
  * @function Rehydration_Pull
  * @brief    Pulls plunger backward (reverse motor) for maintenance/priming
  * @param    uL Volume to retract in microliters
  */
 void Rehydration_Pull(uint32_t uL) {
     DRV8825_Set_Step_Mode(&rehydrationMotor, DRV8825_QUARTER_STEP);
     float uL_per_step = calculate_uL_per_step();
     uint32_t steps = (uint32_t)(uL / uL_per_step);
     printf("Retracting %lu uL (%lu steps)\n", uL, steps);
     DRV8825_Move(&rehydrationMotor, steps, DRV8825_BACKWARD, DRV8825_DEFAULT_STEP_DELAY_US);
 }
 
 /**
  * @function Rehydration_Stop
  * @brief    Stops the motor by disabling output
  */
 void Rehydration_Stop(void) {
     DRV8825_Disable(&rehydrationMotor);
     printf("Rehydration motor stopped.\n");
 }
 


 
//  #define TESTING_REHYDRATION
#ifdef TESTING_REHYDRATION

int main(void) {
    BOARD_Init();
    GPIO_Init();
    TIMER_Init();

    printf("Starting Rehydration Test...\n");
    printf("uL per step %f.5 \n", calculate_uL_per_step());
    Rehydration_Init();

    HAL_Delay(2000); // Wait 10 seconds before starting

    // Test: Push 20 uL of fluid for one tube
    // Rehydration_Push((1000)*12);
    HAL_Delay(2000); // Wait 2 seconds

    Rehydration_Pull(4000);


    // Stop the motor
    Rehydration_Stop();

    printf("Test complete.\n");

    while (1); // Infinite loop to keep program alive
}
#endif
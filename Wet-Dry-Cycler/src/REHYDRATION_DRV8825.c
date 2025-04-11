/**
 * @file    DRV8825.c
 *
 * DRV8825 Stepper Motor Driver Control Module
 *
 * @author  Rafael Delwart
 * @date    10 Apr 2025
 *
 */

 #include <stdio.h>
 #include <stdint.h>
 #include <GPIO.h>
 #include <timers.h>
 #include <REHYDRATION_DRV8825.h>
 


// TESTS *********************************************************
#define DRV8825_TEST


 
 /**
  * @function DRV8825_Init()
  * @brief Initializes DRV8825 driver GPIO pins
  */
 void DRV8825_Init(void) {
     GPIO_Init();
     TIMER_Init(); 
     GPIO_WritePin(DRV8825_STEP_PIN, LOW);
     GPIO_WritePin(DRV8825_DIR_PIN, DRV8825_FORWARD);
     DRV8825_Set_Step_Mode(DRV8825_FULL_STEP);

    if (DRV8825_Check_Fault()) {
        printf("DRV8825 FAULT on INIT: Check wiring or overcurrent\n");
    }
 }
 

 /**
 * @function DRV8825_Check_Fault(void)
 * @brief Returns TRUE if fault is active (LOW), FALSE otherwise
 */
int DRV8825_Check_Fault(void) {
    return (GPIO_ReadPin(DRV8825_FAULT_PIN) == 0); // Active-low signal
}
 
 /**
  * @function DRV8825_Set_Direction(int direction)
  * @brief Sets the motor direction or prints error if invalid
  */
    void DRV8825_Set_Direction(int direction) {
        if (direction == DRV8825_FORWARD) {
            GPIO_WritePin(DRV8825_DIR_PIN, HIGH);
        } 
        else if (direction == DRV8825_BACKWARD) {
            GPIO_WritePin(DRV8825_DIR_PIN, LOW);
        } 
        else {
            printf("DRV8825_Set_Direction ERROR: Invalid direction %d\r\n", direction);
        }
    }
 
 
 /**
  * @function DRV8825_Step()
  * @brief Sends a single step pulse, as fast as possible
  */
 void DRV8825_Step(void) {
    GPIO_WritePin(DRV8825_STEP_PIN, HIGH);
    uint32_t start = TIMERS_GetMicroSeconds();
    while ((TIMERS_GetMicroSeconds() - start) < 2);  // 2 us HIGH pulse (as fast is allow according to data sheet)
    GPIO_WritePin(DRV8825_STEP_PIN, LOW);
    start = TIMERS_GetMicroSeconds();
    while ((TIMERS_GetMicroSeconds() - start) < 2);  // 2 us LOW pulse
}
 
 
 /**
  * @function DRV8825_Step_N(int steps, int delay_us)
  * @brief Sends multiple step pulses with delays
  */
 void DRV8825_Step_N(int steps, int delay_us) {
    for (int i = 0; i < steps; i++) {
        if (DRV8825_Check_Fault()) {
            printf("DRV8825 FAULT DETECTED: Motor stopped at step %d\n", i);
            return;
        }
        DRV8825_Step();
        uint32_t start = TIMERS_GetMicroSeconds();
        while ((TIMERS_GetMicroSeconds() - start) < delay_us);
    }
}
 
 /**
  * @function DRV8825_Move(int steps, int direction, int delay_us)
  * @brief Moves the motor a given number of steps in a direction
  */
 void DRV8825_Move(int steps, int direction, int delay_us) {
    if (DRV8825_Check_Fault()) {
        printf("DRV8825 FAULT DETECTED BEFORE MOVE: Aborting\n");
        return;
    }
     DRV8825_Set_Direction(direction);
     DRV8825_Step_N(steps, delay_us);
 }
 
 
 /**
  * @function DRV8825_Set_Step_Mode(int mode)
  * @brief Sets the microstepping mode using MODE0–2
  */
 void DRV8825_Set_Step_Mode(int mode) {
     int mode0 = mode & 0x01;
     int mode1 = (mode >> 1) & 0x01;
     int mode2 = (mode >> 2) & 0x01;
 
     GPIO_WritePin(DRV8825_MODE0_PIN, mode0);
     GPIO_WritePin(DRV8825_MODE1_PIN, mode1);
     GPIO_WritePin(DRV8825_MODE2_PIN, mode2);
 }
 
 
 #ifdef DRV8825_TEST
 
 int main(void) {
     BOARD_Init();
     TIMER_Init();
     DRV8825_Init();
 
     while (1) {
         printf("Moving forward...\n");
         DRV8825_Move(200, DRV8825_FORWARD, DRV8825_DEFAULT_STEP_DELAY_US);
         uint32_t delay_f = TIMERS_GetMicroSeconds();
         while ((TIMERS_GetMilliSeconds() - delay_f) < 2000);  // 2 second delay between driving forward and back 
         printf("Moving backward...\n");
         DRV8825_Move(200, DRV8825_BACKWARD, DRV8825_DEFAULT_STEP_DELAY_US);
         uint32_t delay_b = TIMERS_GetMicroSeconds();
         while ((TIMERS_GetMilliSeconds() - delay_b) < 2000); 
    }  
}
 
 #endif // DRV8825_TEST
 
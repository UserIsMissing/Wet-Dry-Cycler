
/**
 * @file    Heating.c
 *
 * Silicon Heating Pad control module 
 *
 * @author  Rafael Delwart
 * @date    20 Feb 2025
 *       
 *          
 */

#include <stdio.h>
#include <stdint.h>
#include <Board.h>
#include <PWM.h>
#include <MIXING.h>




// PINOUTS *******************************************************
#define MIXING_CONTROL_PIN PWM_3 //IO SHIELD:56 STM: PA11 - CN10_14, GPIOA, GPIO PIN 11
//GND PIN IO SHIELD: 59
// TESTS *********************************************************
#define TESTING_MIXING


            

/**
 * @function MIXING_Init()
 * 
 * @brief This function initializes the module for use. Initialization is done by initializing the PWM then
 * opening and configuring the PWM pin used to turn the FET on.
 * @author Rafael Delwart, 25 Feb 2025 
 * 
 * 
 * */

void MIXING_Init(void) {
    char initPWMResult = PWM_Init();
    if (initPWMResult != TRUE){
        printf("Initialization of Mixing PWM failed, stopping here\r\n");
    }
    else{
        printf("Mixing PWM Initialization succeeded\r\n");
    }
    PWM_AddPin(MIXING_CONTROL_PIN);
    PWM_SetDutyCycle(MIXING_CONTROL_PIN, 0);  // Initially motor is off
}



/**
 * @function MIXING_Motor_On(void)
 * @param None
 * @return None
 * @brief Turns on the mixing motor
 * @author Rafael Delwart, 25 Feb 2025 
 * 
 * */
void MIXING_Motor_On(void){
    PWM_SetDutyCycle(MIXING_CONTROL_PIN, 100);  // Turn motor on (100% duty cycle)
}


/**
 * @function MIXING_Motor_Off(void)
 * @param None
 * @return None
 * @brief Turns off the mixing motor
 * @author Rafael Delwart, 25 Feb 2025 
 * 
 * */
void MIXING_Motor_Off(void){
    PWM_SetDutyCycle(MIXING_CONTROL_PIN, 0);  // Turn motor off (0% duty cycle)
}

#ifdef TESTING_MIXING

// Simple debounce function (waits for the button to settle)
void debounce() {
    // Small delay (typically 10-20ms) to debounce the button
    for (volatile int i = 0; i < 100000; i++);  // Adjust this delay as needed
}

int main(void)
{
    BOARD_Init();
    I2C_Init();
    MIXING_Init();
    BUTTONS_Init();

    while (1)
    {
        uint8_t buttonState = buttons_state();  // Get current button states

        // Check if Button 4 is pressed (bit 3 of the state)
        if ((buttonState & 0x08) == 0)  // Button 4 pressed
        {
            MIXING_Motor_On();
            printf("Motor is now ON with 100%% duty cycle.\n");
            debounce();  // Add debounce delay to avoid multiple detections
        }
        // Check if Button 3 is pressed (bit 2 of the state)
        else if ((buttonState & 0x04) == 0 )  // Button 3 pressed
        {
            MIXING_Motor_Off();           
            printf("Motor is OFF.\n");
            debounce();  // Add debounce delay to avoid multiple detections
        }
    }
}
#endif // TESTING_MIXING


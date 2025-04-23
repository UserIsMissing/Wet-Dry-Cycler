/**
 * @file    MIXING.c
 *
 * Motor control using GPIO PWM pulses. This version uses a simplified
 * software PWM interface where each pin is toggled with manual duty and frequency.
 *
 * @author  Rafael Delwart
 * @date    22 Apr 2025
 */

 #include <stdio.h>
 #include <Board.h>
 #include <MIXING.h>
 #include <GPIO.h>

 // Define the pins to control
 static const uint8_t motorPins[NUM_MOTOR_PINS] = {PIN_C8, PIN_C9, PIN_B1};
 
 /**
  * @function MIXING_Init()
  * @brief Initializes the motor pins (optional GPIO config)
  */
 void MIXING_Init(void) {
     // Optional: configure GPIO pins as output if needed here
     for (int i = 0; i < NUM_MOTOR_PINS; i++) {
        GPIO_WritePin(motorPins[i], LOW);
     }
 
     printf("All motors initialized and set to OFF.\r\n");
 }
 
 /**
  * @function MIXING_Motor_OnPin(uint8_t pin)
  * @brief Turns on the motor connected to the specified GPIO pin
  */
 void MIXING_Motor_OnPin(uint8_t pin){
    GPIO_WritePin(pin, HIGH);  // Set pin HIGH
 }
 
 /**
  * @function MIXING_Motor_OffPin(uint8_t pin)
  * @brief Turns off the motor connected to the specified GPIO pin
  */
 void MIXING_Motor_OffPin(uint8_t pin){
    GPIO_WritePin(pin, LOW);  // Set pin LOW
 }
 
 /**
  * @function MIXING_AllMotors_On()
  * @brief Turns on all defined motor pins
  */
 void MIXING_AllMotors_On(void){
     for (int i = 0; i < NUM_MOTOR_PINS; i++) {
        GPIO_WritePin(motorPins[i], HIGH);  // Set pins HIGH
    }
 }
 
 /**
  * @function MIXING_AllMotors_Off()
  * @brief Turns off all defined motor pins
  */
 void MIXING_AllMotors_Off(void){
     for (int i = 0; i < NUM_MOTOR_PINS; i++) {
        GPIO_WritePin(motorPins[i], LOW);  // Set pins LOW
     }
 }
 
//  #define TESTING_MIXING 
 #ifdef TESTING_MIXING
 int main(void)
 {
     HAL_Init();         // Initialize HAL
     BOARD_Init();       // Board-specific setup
     MIXING_Init();      // Initialize motor pins
 
     while (1)
     {
        // GPIO_WritePin(PIN_C8, HIGH);//  // Cycle through motors one at a time
        // GPIO_WritePin(PIN_C9, HIGH);//  // Cycle through motors one at a time
        // GPIO_WritePin(PIN_B2, HIGH);//  // Cycle through motors one at a time

         for (int i = 0; i < NUM_MOTOR_PINS; i++) {
             printf("Turning ON motor %d\r\n", i + 1);
             MIXING_Motor_OnPin(motorPins[i]);
             HAL_Delay(10000); // Delay 1 second
 
             printf("Turning OFF motor %d\r\n", i + 1);
             MIXING_Motor_OffPin(motorPins[i]);
             HAL_Delay(5000);  // Delay 0.5 second
         }
 
         //Turn all motors ON and OFF
         printf("Turning ALL motors ON\r\n");
         MIXING_AllMotors_On();
         HAL_Delay(20000);  // Delay 2 seconds
 
         printf("Turning ALL motors OFF\r\n");
         MIXING_AllMotors_Off();
         HAL_Delay(20000);  // Delay 2 seconds
     }
 
     return 0;
 }
 #endif
// #include "main.h"  // Adjust this include as needed for your specific project setup
#include "stm32f4xx_hal.h"
#include <stdlib.h> // For abs()

#define NUM_STEPS 8      // Number of steps for your stepper motor (adjust for your motor)
#define STEP_DELAY_MS 5 // Delay between steps in milliseconds

int step_number = 0;         // Current step position
uint32_t last_step_time = 0; // Last time a step was taken

// Define the stepper motor pin control (change these as per your hardware setup)
#define MOTOR_PIN_1 GPIO_PIN_8 // Example pins, adjust to actual ones
#define MOTOR_PIN_2 GPIO_PIN_9
#define MOTOR_PIN_3 GPIO_PIN_10
#define MOTOR_PIN_4 GPIO_PIN_11

// Set GPIO pins for the current step (this function depends on your wiring)
void Stepper_SetStep(int step)
{
    // GPIO logic here to set the pins according to the current step
    // Example: (This is just for demonstration, update as needed)
    switch (step)
    {
    case 0:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_RESET);
        break;
    case 1:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_RESET);
        break;
    case 2:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_RESET);
        break;
    case 3:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_RESET);
        break;
    case 4:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_RESET);
        break;
    case 5:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_SET);
        break;
    case 6:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_SET);
        break;
    case 7:
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_1, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_3, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, MOTOR_PIN_4, GPIO_PIN_SET);
        break;
    // Add cases for more steps if needed (for different stepper modes)
    default:
        break;
    }
}

// Function to move the stepper motor by a specified number of steps
void Stepper_Step(int steps_to_move)
{
    int steps_left = abs(steps_to_move);
    int direction = (steps_to_move > 0) ? 1 : -1; // Determine direction

    while (steps_left > 0)
    {
        uint32_t current_time = HAL_GetTick(); // Get current time in milliseconds

        // If enough time has passed to take another step
        if (current_time - last_step_time >= STEP_DELAY_MS)
        {
            // Update step number based on direction
            step_number = (step_number + direction + NUM_STEPS) % NUM_STEPS;

            // Set the motor pins for the current step
            Stepper_SetStep(step_number);

            steps_left--;                  // Decrease steps remaining
            last_step_time = current_time; // Update last step time
        }
    }
}

int main(void)
{
    // HAL initialization code
    HAL_Init(); // Initialize the Hardware Abstraction Layer

    // GPIO initialization (example for STM32F4 series, adapt to your MCU)
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Enable GPIOA clock (adjust if different)

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = MOTOR_PIN_1 | MOTOR_PIN_2 | MOTOR_PIN_3 | MOTOR_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); // Initialize GPIO pins

    while (1)
    {
        // Call the Stepper_Step function to move the motor
        Stepper_Step(10); // Move 10 steps forward
        // Add other tasks here (non-blocking) as needed
        HAL_Delay(100); // Just an example delay to simulate a non-blocking task
    }
}

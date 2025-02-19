// INCLUDES ******************************************************
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>

// PINOUTS *******************************************************
#define Temperature_Pin ADC_0

// TESTS *********************************************************
#define TESTING_TEMP

// DEFINES *******************************************************
#define MOVING_AVERAGE_WINDOW 80  // Number of samples for the moving average

// VARIABLES *****************************************************
float adcBuffer[MOVING_AVERAGE_WINDOW];  // Circular buffer for ADC readings
int adcIndex = 0;  // Index for the circular buffer
float adcSum = 0;  // Sum of the values in the buffer
int samplesCollected = 0;  // Counter to track how many samples have been collected

// FUNCTION PROTOTYPES ********************************************
float calculateMovingAverage(float newAdcValue);

// ***************************************************************

int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();

    while (1)
    {
        char initResult = ADC_Init();

        if (initResult != TRUE)
        {
            printf("Initialization of IMU failed, stopping here\r\n");
        }
        else
        {
            printf("Initialization succeeded\r\n");
            int i = 0;

            // float Resistance = 283000* (2-ADC_Read(Temperature_Pin))/ADC_Read(Temperature_Pin);
            float R1 = 283000; // 283kOhm
            
            while (TRUE)
            {           
                #ifdef TESTING_TEMP
                int rawAdcValue = ADC_Read(Temperature_Pin);
                float movingAverageAdc = calculateMovingAverage((float)rawAdcValue);

                float Voltage = (3.3 * (movingAverageAdc / 4096));
                float Resistance = R1 * (2 - Voltage) / Voltage;
                float Temperature = 106.91609 * exp(-0.00001378 * Resistance);

                printf(">Raw Temperature: %d\n", rawAdcValue);
                printf(">Voltage: %0.3f\n", Voltage);
                printf(">Resistance: %0.3f\n", Resistance / 1000);    // kOhm
                printf(">Temperature: %0.3f\n", Temperature);     // Celsius
                // printf(">Moving Average: %0.3f\n", movingAverageAdc);
                #endif // TESTING_TEMP
                
                i++;
            }
        }
    }
}

// FUNCTION DEFINITIONS *******************************************

float calculateMovingAverage(float newAdcValue)
{
    // If the buffer is full, reset it
    if (samplesCollected >= MOVING_AVERAGE_WINDOW)
    {
        // Reset the buffer and sum
        for (int i = 0; i < MOVING_AVERAGE_WINDOW; i++)
        {
            adcBuffer[i] = 0;
        }
        adcSum = 0;
        adcIndex = 0;
        samplesCollected = 0;
    }

    // Add the new value to the buffer and update the sum
    adcSum -= adcBuffer[adcIndex];  // Subtract the oldest value
    adcBuffer[adcIndex] = newAdcValue;  // Store the new value
    adcSum += newAdcValue;  // Add the new value to the sum

    // Update the index for the next value
    adcIndex = (adcIndex + 1) % MOVING_AVERAGE_WINDOW;

    // Increment the sample counter
    samplesCollected++;

    // Return the average
    return adcSum / samplesCollected;
}
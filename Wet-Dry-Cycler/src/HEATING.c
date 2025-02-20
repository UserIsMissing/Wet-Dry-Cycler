
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
#include "ADC.h"
#include <math.h>



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
float R1 = 283000; // 283kOhm  float Resistance = 283000* (2-ADC_Read(Temperature_Pin))/ADC_Read(Temperature_Pin);
            

/**
 * @function HEATING_Init()
 * 
 * @brief Initializes the ADC then opens and configures the ADC pin used to measure the voltage from the voltage divider
 * @author Rafael Delwart, 20 Feb 2025 
 * 
 * 
 * */

void HEATING_Init(void) {
    char initResult = ADC_Init();
    if (initResult != TRUE){
        printf("Initialization of Heating ADC failed, stopping here\r\n");
    }
    else{
        printf("Heating ADC Initialization succeeded\r\n");
    }

}


/** calculateMovingAverage(float newAdcValue)
 *
 * Implements a moving average filter for ADC readings
 *
 * @return  (float)  [Filtered Values]
 */
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





/** HEATING_Measure_Raw_ADC(void)
 *
 * Returns the measured raw ADC.
 *
 * @return  (int)  [raw ADC]
 */
int HEATING_Measure_Raw_ADC(void){
    int rawAdcValue = ADC_Read(Temperature_Pin);
    return rawAdcValue;
}

/** HEATING_Measure_Voltage(void)
 *
 * Returns the current temperature of the heating pad in degrees Celsius
 *
 * @return  (float)  [Volts]
 */
float HEATING_Measure_Voltage(void){
    float movingAverageAdc = calculateMovingAverage((float)HEATING_Measure_Voltage());
    float Voltage = (3.3 * (movingAverageAdc / 4096));
    return Voltage;
}


/** HEATING_Measure_Resistance(void)
 *
 * Returns the current resistance of the thermistor pad in Ohms
 *
 * @return  (float)  [Ohms]
 */
float HEATING_Measure_Resistance(void){
    float Voltage = HEATING_Measure_Voltage();
    float Resistance = R1 * (2 - Voltage) / Voltage;
    return Resistance;


}


/**
 * @function HEATING_Measure_Temp(void)
 * @param None
 * @return  (float)  [degrees Celsius]
 * @brief Returns the current temperature of the heating pad in degrees Celsius
 * @author Rafael Delwart, 20 Feb 2025 */
float HEATING_Measure_Temp(void) {
    float Temperature = 106.91609 * exp(-0.00001378 * HEATING_Measure_Resistance());
    return Temperature;
}











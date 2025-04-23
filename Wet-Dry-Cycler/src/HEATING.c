
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
#include <ADC.h>
#include <math.h>
#include <GPIO.h>
#include <HEATING.h>




// DEFINES *******************************************************
#define MOVING_AVERAGE_WINDOW 80  // Number of samples for the moving average

// VARIABLES *****************************************************
const float R0 = 100000.0;          // Resistance at T0 (25°C)
float R1 = 4630; // 283kOhm  float Resistance = 283000* (2-ADC_Read(Temperature_Pin))/ADC_Read(Temperature_Pin);
float VIn = 3.3; // 2V  Input voltage to the voltage divider 
const float BETA = 3950.0;         // Beta value of thermistor
const float T0 = 298.15;           // 25°C in Kelvin

//MOVING AVERAGE
static int adcBuffer[MOVING_AVERAGE_WINDOW] = {0};  // Circular buffer for ADC moving average
static int adcIndex = 0;  // Index for circular buffer
static int adcCount = 0;  // Track how many ADC samples have been collected

static float tempBuffer[MOVING_AVERAGE_WINDOW] = {0};  // Circular buffer for temperature moving average
static int tempIndex = 0;  // Index for circular buffer
static int tempCount = 0;  // Track how many temperature samples have been collected

/**
 * @function HEATING_Init()
 * 
 * @brief Initializes the ADC then opens and configures the ADC pin used to measure the voltage from the voltage divider
 * @author Rafael Delwart, 20 Feb 2025 
 * 
 * 
 * */

void HEATING_Init(void) {
    char initADCResult = ADC_Init();
    if (initADCResult != TRUE){
        printf("Initialization of Heating ADC failed, stopping here\r\n");
    }
    else{
        printf("Heating ADC Initialization succeeded\r\n");
    }
    char initPWMResult = PWM_Init();
    if (initPWMResult != TRUE){
        printf("Initialization of Heating PWM failed, stopping here\r\n");
    }
    else{
        printf("Heating PWM Initialization succeeded\r\n");
    }
    PWM_AddPin(HEATING_CONTROL_PIN);
    PWM_SetDutyCycle(HEATING_CONTROL_PIN, 0);  // Initially Pad is off

}





/** HEATING_Measure_Raw_ADC(void)
 *
 * Returns the measured raw ADC.
 *
 * @return  (int)  [raw ADC]
 */
int HEATING_Measure_Raw_ADC(void){
    int rawAdcValue = ADC_Read(THERMISTOR_PIN);
    return rawAdcValue;
}


/**
 * @function HEATING_Measure_Raw_ADC_Avg(void)
 * @brief Returns the moving average of the raw ADC readings.
 * @return (int) [raw ADC]
 * @author Rafael Delwart, 1 Mar 2025
 */
int HEATING_Measure_Raw_ADC_Avg(void) {
    int newAdc = HEATING_Measure_Raw_ADC();  // Get current ADC reading
    adcBuffer[adcIndex] = newAdc;  // Store in buffer
    adcIndex = (adcIndex + 1) % MOVING_AVERAGE_WINDOW;  // Update circular index

    if (adcCount < MOVING_AVERAGE_WINDOW) {
        adcCount++;  // Ensure we don't divide by zero before buffer fills up
    }

    // Compute moving average
    int sum = 0;
    for (int i = 0; i < adcCount; i++) {
        sum += adcBuffer[i];
    }
    return sum / adcCount;
}


/** HEATING_Measure_Voltage(void)
 *
 * Returns the current temperature of the heating pad in degrees Celsius
 *
 * @return  (float)  [Volts]
 */
float HEATING_Measure_Voltage(void){
    float Voltage = (3.3 * (float)((float)HEATING_Measure_Raw_ADC_Avg() / 4096));
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
    float Resistance = R1 * (VIn - Voltage) / Voltage;
    return Resistance;


}


// /**
//  * @function HEATING_Measure_Temp(void)
//  * @param None
//  * @return  (float)  [degrees Celsius]
//  * @brief Returns the current temperature of the heating pad in degrees Celsius
//  * @author Rafael Delwart, 20 Feb 2025 */
// float HEATING_Measure_Temp(void) {
//     float Temperature = 106.91609 * exp(-0.00001378 * HEATING_Measure_Resistance());
//     return Temperature;
// }   OLD  FUNCTION USING MANUAL REGRESSION

/**
 * @function HEATING_Measure_Temp(void)
 * @param None
 * @return  (float)  [degrees Celsius]
 * @brief Calculates the temperature using the BETA method with BETA = 3950.
 *        Returns the current temperature of the heating pad in degrees Celsius.
 * @author Rafael Delwart, 20 Feb 2025
 */
float HEATING_Measure_Temp(void) {

    float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(HEATING_Measure_Resistance() / R0));
    float tempC = tempK - 273.15;      // Convert to Celsius

    return tempC;
}



/**
 * @function HEATING_Measure_Temp_Avg(void)
 * @brief Returns the moving average temperature of the heating pad
 * @return (float) [degrees Celsius]
 * @author Rafael Delwart, 1 Mar 2025
 */
float HEATING_Measure_Temp_Avg(void) {
    float newTemp = HEATING_Measure_Temp();  // Get current temperature
    tempBuffer[tempIndex] = newTemp;  // Store in buffer
    tempIndex = (tempIndex + 1) % MOVING_AVERAGE_WINDOW;  // Update circular index

    if (tempCount < MOVING_AVERAGE_WINDOW) {
        tempCount++;  // Ensure we don't divide by zero before buffer fills up
    }

    // Compute moving average
    float sum = 0;
    for (int i = 0; i < tempCount; i++) {
        sum += tempBuffer[i];
    }
    return sum / tempCount;
}

/**
 * @function HEATING_Set_Temp(int Temp)
 * @param Int
 * @return  None
 * @brief Implements simple bang bang control for the heating pad
 * @author Rafael Delwart, 1 Mar 2025 */
void HEATING_Set_Temp(int Temp){
    if(HEATING_Measure_Temp_Avg() < (float)Temp){
        GPIO_WritePin(HEATING_CONTROL_PIN, HIGH); // Turn Heating Pad on (100% duty cycle)
        printf("Heating Pad Turned On");
    }
    else{
        GPIO_WritePin(HEATING_CONTROL_PIN, LOW);  // Turn Heating Pad Off (0% duty cycle)
        printf("Heating Pad Turned Off");
    }

}



#define TESTING_TEMP

#ifdef TESTING_TEMP

int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();
    HEATING_Init();

    while (1)
    {
        printf(">Raw ADC: %d\n", HEATING_Measure_Raw_ADC());
        printf(">Raw ADC: %d\n", HEATING_Measure_Raw_ADC());
        printf(">ADC AVG: %d\n", HEATING_Measure_Raw_ADC_Avg());
        printf(">Voltage: %0.3f\n", HEATING_Measure_Voltage());
        printf(">Resistance: %0.3f\n", HEATING_Measure_Resistance() / 1000);    // kOhm
        printf(">Temperature: %0.3f\n", HEATING_Measure_Temp());     // Celsius
        printf(">Temperature AVG: %0.3f\n", HEATING_Measure_Temp_Avg());     // Celsius

        HEATING_Set_Temp(50);
    }
}
#endif // TESTING_TEMP






// INCLUDES ******************************************************
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>



// ***************************************************************

int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();

    while (1)
    {
      
        #ifdef TESTING_TEMP
        int rawAdcValue = ADC_Read(Temperature_Pin);
        float movingAverageAdc = calculateMovingAverage((float)rawAdcValue);

        float Voltage = (3.3 * (movingAverageAdc / 4096));
        float Resistance = R1 * (2 - Voltage) / Voltage;
        float Temperature = 106.91609 * exp(-0.00001378 * Resistance);

        printf(">Raw ADC: %d\n", HEATING_Measure_Raw_ADC());
        printf(">Voltage: %0.3f\n", HEATING_Measure_Voltage());
        printf(">Resistance: %0.3f\n", HEATING_Measure_Resistance() / 1000);    // kOhm
        printf(">Temperature: %0.3f\n", HEATING_Measure_Temp());     // Celsius
        #endif // TESTING_TEMP
                
    }
}


// FUNCTION DEFINITIONS *******************************************


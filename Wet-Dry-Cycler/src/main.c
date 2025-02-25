// INCLUDES ******************************************************
#include <stdio.h>
#include <stdlib.h>
#include "Board.h"
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>
#include <HEATING.h>

#define TESTING_TEMP

// ***************************************************************

int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();
    HEATING_Init();

    #ifdef TESTING_TEMP

    while (1)
    {
      
        printf(">Raw ADC: %d\n", HEATING_Measure_Raw_ADC());
        printf(">Voltage: %0.3f\n", HEATING_Measure_Voltage());
        printf(">Resistance: %0.3f\n", HEATING_Measure_Resistance() / 1000);    // kOhm
        printf(">Temperature: %0.3f\n", HEATING_Measure_Temp());     // Celsius
                
    }
    #endif // TESTING_TEMP

}


// FUNCTION DEFINITIONS *******************************************


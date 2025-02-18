// INCLUDES ******************************************************
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>
// #include <pwm.h>
// #include <buttons.h>
// #include <leds.h>
// PINOUTS *******************************************************
#define Temperature_Pin ADC_0
// TESTS *********************************************************
#define TESTING_TEMP
// DEFINES *******************************************************

// VARIABLES *****************************************************

// ***************************************************************


int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();

    while (1)
    {
        char initResult = BNO055_Init();

        if (initResult != TRUE)
        {
            printf("Initialization of IMU failed, stopping here\r\n");
        }
        else
        {
            printf("Initialization succeeded\r\n");
            int i = 0;

            while (TRUE)
            {           
                #ifdef TESTING_TEMP
                printf(">Raw Temperature: %d\n", ADC_Read(Temperature_Pin));
                #endif // TESTING_TEMP
                i++;
            }
        }
    }
}

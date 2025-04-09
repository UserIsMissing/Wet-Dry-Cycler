#include <stdio.h>
#include <stdlib.h>
#include "Board.h"
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>
#include <HEATING.h>
#include <MIXING.h>
#include <PWM.h>
#include <buttons.h>
#include <GPIO.h>


#define TESTING_MAIN

// ***************************************************************

#ifdef TESTING_MAIN


int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();
    PWM_Init();
    PWM_AddPin(PWM_0);
    PWM_SetFrequency(1000);
    GPIO_Init();
    GPIO_WritePin(PIN_0, HIGH);


    while(1){
        printf("printing\n");
        PWM_SetDutyCycle(PWM_0, 90);

        GPIO_WritePin(PIN_3, HIGH);
    }
}

#endif // TESTING_MAIN


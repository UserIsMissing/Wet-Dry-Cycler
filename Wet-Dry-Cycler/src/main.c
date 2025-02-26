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


// #define TESTING_MAIN

// ***************************************************************

#ifdef TESTING_MAIN


int main(void)
{
    BOARD_Init();
    I2C_Init();
    TIMER_Init();
    ADC_Init();

    while(1){


    }
}

#endif // TESTING_MAIN


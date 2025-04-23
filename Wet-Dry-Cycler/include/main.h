#pragma once
#include "ADC.h"
#include "Board.h"
#include "DRV8825.h"
#include "GPIO.h"
#include "HEATING.h"
#include "I2C.h"
#include "MIXING.h"
#include "MOVEMENT.h"
#include "REHYDRATION.h"
#include "buttons.h"
#include "math.h"
#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "stdlib.h"
#include "stdio.h"
#include "timers.h"

#define TESTING_ISR
// #define TESTING_TEMP
// #define TESTING_REHYDRATION
// #define TESTING_MOVEMENT
// #define TESTING_MIXING
#define TESTING_MAIN

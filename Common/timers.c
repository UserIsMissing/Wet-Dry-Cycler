/*
 * File:   timers.c
 * Author: Adam Korycki
 *
 * Created on September 29, 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>

// Boolean defines for TRUE, FALSE, SUCCESS and ERROR
#ifndef FALSE
#define FALSE ((int8_t) 0)
#define TRUE ((int8_t) 1)
#endif
#ifndef ERROR
#define ERROR ((int8_t) -1)
#define SUCCESS ((int8_t) 1)
#endif

static uint8_t init_status = FALSE;

static uint32_t us; //microsecond count
static uint32_t ms; //millisecond count

/**
 * @function TIMER_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief Initializes and starts the timer (TIM2) peripheral
 * @author Adam Korycki, 2023.09.29 */
char TIMER_Init(void) {
    if (init_status == FALSE) { // if TIM2 module has not been initialized
        TIM_ClockConfigTypeDef sClockSourceConfig = {0};
        TIM_MasterConfigTypeDef sMasterConfig = {0};

        uint32_t system_clock_freq = TIMERS_GetSystemClockFreq() / 1000000; // system clock freq in Mhz
        htim2.Instance = TIM2;
        htim2.Init.Prescaler = system_clock_freq - 1; // setting prescaler for 1 Mhz timer clock
        htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim2.Init.Period = 999; //trigger interrupt every 1ms
        htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
        {
            return ERROR;
        }
        sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
        if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
        {
            return ERROR;
        }
        sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
        {
            return ERROR;
        }

        HAL_TIM_Base_Start_IT(&htim2); // start interrupt
        init_status = TRUE;
    }
    return SUCCESS;
}

/**
 * @function TIMERS_GetMilliSeconds(void)
 * @param None
 * @return current millisecond count
 * @brief ^
 * @author Adam Korycki, 2023.09.29 */
uint32_t TIMERS_GetMilliSeconds(void) {
    return ms;
}

/**
 * @function TIMERS_GetMicroSeconds(void)
 * @param None
 * @return current microsecond count
 * @brief ^
 * @author Adam Korycki, 2023.09.29 */
uint32_t TIMERS_GetMicroSeconds(void) {
    return us + TIM2->CNT; // (ms*1000) + (current value of 1Mhz counter)
}

/**
 * @function TIMERS_GetSystemClockFreq(void)
 * @param None
 * @return frequnecy of system clock in hz
 * @brief ^
 * @author Adam Korycki, 2023.09.29 */
uint32_t TIMERS_GetSystemClockFreq(void) {
    return HAL_RCC_GetSysClockFreq();
}

/* timer callback */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim2) {
        ms++;     // update millisecond count
        us+=1000; // update microsecond count
    }
}

//#define TIMERS_TEST
#ifdef TIMERS_TEST // TIMERS TEST HARNESS
//SUCCESS - printed microsecond and millisecond values are are around 100 ms apart

#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <timers.h>


int main(void) {
    BOARD_Init();
    TIMER_Init();

    uint32_t init_ms = TIMERS_GetMilliSeconds();
    uint32_t init_us = TIMERS_GetMicroSeconds();
    while(TRUE) {
        printf("ms: %011d\r\nus: %011d\r\n\r\n", TIMERS_GetMilliSeconds()-init_ms, TIMERS_GetMicroSeconds()-init_us);
        HAL_Delay(1);
    }
}
#endif
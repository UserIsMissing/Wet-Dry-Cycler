/*
 * File:   pwm.c
 * Author: Adam Korycki
 *
 * Created on October 5, 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <timers.h>
#include <pwm.h>

// Boolean defines for TRUE, FALSE, SUCCESS and ERROR
#ifndef FALSE
#define FALSE ((int8_t) 0)
#define TRUE ((int8_t) 1)
#endif
#ifndef ERROR
#define ERROR ((int8_t) -1)
#define SUCCESS ((int8_t) 1)
#endif

#define NUM_CHANNELS 6 // number of pwm possible channels

// User-level PWM channels for inits/updating duty cycle etc...
const PWM PWM_0 = {&htim1, TIM_CHANNEL_1, 0x1};
const PWM PWM_1 = {&htim1, TIM_CHANNEL_2, 0x2};
const PWM PWM_2 = {&htim1, TIM_CHANNEL_3, 0x4};
const PWM PWM_3 = {&htim1, TIM_CHANNEL_4, 0x8};
const PWM PWM_4 = {&htim4, TIM_CHANNEL_1, 0x10};
const PWM PWM_5 = {&htim4, TIM_CHANNEL_3, 0x20};

static unsigned int pwm_freq = 1000; // [1 khz] default frequency 
static uint32_t duty_cycles[NUM_CHANNELS]; // to store the duty cycles of each channel
static uint8_t init_status = FALSE;
static unsigned char pinsAdded = 0x00;

/**
 * @Function PWM_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief  Initializes the timer for the PWM system and set is to the default frequency of 1 khz
 * @author Adam Korycki, 2023.10.05 */
char PWM_Init(void) {
    if (init_status == FALSE) { // if PWM module has not been inited
        // init TIM1
        TIM_ClockConfigTypeDef sClockSourceConfig = {0};
        TIM_MasterConfigTypeDef sMasterConfig = {0};

        uint32_t system_clock_freq = TIMERS_GetSystemClockFreq() / 1000000; // system clock freq in Mhz
        htim1.Instance = TIM1;
        htim1.Init.Prescaler = system_clock_freq - 1; // setting prescaler for 1 Mhz timer clock
        htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim1.Init.Period = 999; // deafault frequecy of 1 khz, changed by modifying ARRx register
        htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim1.Init.RepetitionCounter = 0;
        htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
        {
            return ERROR;
        }
        sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
        if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
        {
            return ERROR;
        }
        if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
        {
            return ERROR;
        }
        sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
        {
            return ERROR;
        }

        // init TIM4
        htim4.Instance = TIM4;
        htim4.Init.Prescaler = system_clock_freq - 1; // setting prescaler for 1 Mhz timer clock
        htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
        htim4.Init.Period = 999; // deafault frequecy of 1 khz, changed by modifying ARRx register
        htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
        {
            return ERROR;
        }
        sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
        if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
        {
            return ERROR;
        }
        if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
        {
            return ERROR;
        }
        sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
        {
            return ERROR;
        }
        init_status = TRUE;
    }
    return SUCCESS;
}

/**
 * @Function PWM_AddPin(struct PWM PWM_x)
 * @param PWM_x - PWM channel to enable and configure
 * @return SUCCESS OR ERROR
 * @brief  Adds new pin to the PWM system.
 * @author Adam Korycki, 2023.10.05 */
char PWM_AddPin(PWM PWM_x) {
    if (init_status == FALSE) { // if pwm module has not been initialized
        printf("ERROR: PWM module has not yet been initialized!\r\n");
        return ERROR;
    }
    if ((pinsAdded & PWM_x.mask) != 0) { // if pin has already been added return ERROR
        printf("ERROR: this pwm pin has already been added!\r\n");
        return ERROR;
    }
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(PWM_x.timer, &sConfigOC, PWM_x.channel) != HAL_OK) {
        return ERROR;
    }
    pinsAdded = pinsAdded | PWM_x.mask; // record added pin for book keeping
    HAL_TIM_MspPostInit(PWM_x.timer);
    HAL_TIM_PWM_Start(PWM_x.timer, PWM_x.channel);
    //PWM_SetDutyCycle(PWM_x, 50); // init to 50% DC
    return SUCCESS;
}

/**
 * @Function PWM_SetFrequency(unsigned int NewFrequency)
 * @param NewFrequency - new frequency to set. must be between 100 hz and 100 khz
 * @return SUCCESS OR ERROR
 * @brief  Changes the frequency of the PWM system.
 * @note  Behavior of PWM channels during Frequency change is undocumented
 * @author Adam Korycki, 2023.10.05 */
char PWM_SetFrequency(unsigned int NewFrequency) {
    if (init_status == FALSE) { // if pwm module has not been initialized
        printf("ERROR: PWM module has not yet been initialized!\r\n");
        return ERROR;
    }
    if ((NewFrequency < 100) || (NewFrequency > 100000)) { // if requested frequency is out of bounds
        return ERROR;
    }
    
    TIM1->ARR = TIM4->ARR = (uint32_t)(1000000.0/NewFrequency) - 1; // set auto-reload registers (ARR) accordingly (1 Mhz timer)
    pwm_freq = NewFrequency;

    // update to preserve duty cycle after frequency change
    unsigned char mask = 1;
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if ((pinsAdded & mask) != 0) { // if pin i has been added, update duty cycle with new frequency
            switch(i) {
                case 0:
                    PWM_SetDutyCycle(PWM_0, duty_cycles[i]);
                    break;
                case 1:
                    PWM_SetDutyCycle(PWM_1, duty_cycles[i]);
                    break;
                case 2:
                    PWM_SetDutyCycle(PWM_2, duty_cycles[i]);
                    break;
                case 3:
                    PWM_SetDutyCycle(PWM_3, duty_cycles[i]);
                    break;
                case 4:
                    PWM_SetDutyCycle(PWM_4, duty_cycles[i]);
                    break;
                case 5:
                    PWM_SetDutyCycle(PWM_5, duty_cycles[i]);
                    break;
            }
        }
        mask = mask << 1; // shift bitmask
    }

    return SUCCESS;
}

/**
 * @Function PWM_GetFrequency(void)
 * @return Frequency of system in Hertz
 * @brief  gets the frequency of the PWM system.
 * @author Adam Korycki, 2023.10.05 */
unsigned int PWM_GetFrequency(void) {
    return pwm_freq;
}

/**
 * Function  PWM_SetDutyCycle
 * @param PWM_x - PWM channel to start and set duty cyle
 * @param Duty - duty cycle for the channel (0-100)
 * @return SUCCESS or ERROR
 * @remark Enables the pwm pin if not already enabled and sets the Duty Cycle for a Single Channel
 * @author Adam Korycki, 2023.10.05  */
char PWM_SetDutyCycle(PWM PWM_x, unsigned int Duty) {
    if (init_status == FALSE) { // if pwm module has not been initialized
        printf("ERROR: PWM module has not yet been initialized!\r\n");
        return ERROR;
    }
    if ((pinsAdded & PWM_x.mask) == 0) { // if pin has not been added, add pin
        PWM_AddPin(PWM_x);
    }
    if ((Duty < 0) || (Duty > 100)) { // if requested duty cycle is out of bounds
        printf("ERROR: pwm duty cycle must be between 0 and 100\r\n");
        return ERROR;
    }
    
    switch(PWM_x.mask) { // set capture compare register (CCR) to correct value and save duty cycle value
        case 0x1: // PWM_0
            TIM1->CCR1 = (uint32_t)((Duty/100.0)*(TIM1->ARR));
            duty_cycles[0] = Duty;
            break;
        case 0x2: // PWM_1
            TIM1->CCR2 = (uint32_t)((Duty/100.0)*(TIM1->ARR));
            duty_cycles[1] = Duty;
            break;
        case 0x4: // PWM_2
            TIM1->CCR3 = (uint32_t)((Duty/100.0)*(TIM1->ARR));
            duty_cycles[2] = Duty;
            break;
        case 0x8: // PWM_3
            TIM1->CCR4 = (uint32_t)((Duty/100.0)*(TIM1->ARR));
            duty_cycles[3] = Duty;
            break;
        case 0x10: // PWM_4
            TIM4->CCR1 = (uint32_t)((Duty/100.0)*(TIM1->ARR));
            duty_cycles[4] = Duty;
            break;
        case 0x20: // PWM_5
            TIM4->CCR3 = (uint32_t)((Duty/100.0)*(TIM1->ARR));
            duty_cycles[5] = Duty;
            break;
    }

    return SUCCESS;
}

/**
 * Function: PWM_Start
 * @param PWM_x - PWM channel to start
 * @return SUCCESS or ERROR
 * @remark Starts pwm channel. Used to bring back up pwm channles after PWM_Stop()
 *         Must call PWM_AddPin() or PWM_SetDutyCycle() before using this function.
 * @author Adam Korycki, 2023.11.27  */
char PWM_Start(PWM PWM_x) {
    if (init_status == FALSE) { // if pwm module has not been initialized
        printf("ERROR: PWM module has not yet been initialized!\r\n");
        return ERROR;
    }
    if ((pinsAdded & PWM_x.mask) == 0) { // if pin has not been added
        printf("ERROR: PWM pin has not been added!\r\n");
        return ERROR;
    }
    // start pwm channel
    HAL_TIM_PWM_Start(PWM_x.timer, PWM_x.channel);
    return SUCCESS;
}

/**
 * Function: PWM_Stop
 * @param None
 * @return SUCCESS or ERROR
 * @remark Stops pwm channel. Use PWM_Start() to start channel again
 *         Must call PWM_AddPin() or PWM_SetDutyCycle() before using this function.
 * @author Adam Korycki, 2023.11.27  */
char PWM_Stop(PWM PWM_x) {
    if (init_status == FALSE) { // if pwm module has not been initialized
        printf("ERROR: PWM module has not yet been initialized!\r\n");
        return ERROR;
    }
    if ((pinsAdded & PWM_x.mask) == 0) { // if pin has not been added
        printf("ERROR: PWM pin has not been added!\r\n");
        return ERROR;
    }
    // start pwm channel
    HAL_TIM_PWM_Stop(PWM_x.timer, PWM_x.channel);
    return SUCCESS;
}

/**
 * Function: PWM_End
 * @param None
 * @return SUCCESS or ERROR
 * @remark Deinitializes the PWM system
 * @author Adam Korycki, 2023.10.05  */
char PWM_End(void) {
    if (init_status == FALSE) { // if pwm module has not been initialized
        printf("ERROR: PWM module has not yet been initialized!\r\n");
        return ERROR;
    }
    // stop all pwm channels
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);

    //deinitialize timer peripherals
    HAL_TIM_PWM_DeInit(&htim1);
    HAL_TIM_PWM_DeInit(&htim4);
    HAL_TIM_Base_DeInit(&htim1);
    HAL_TIM_Base_DeInit(&htim4);
    return SUCCESS;
}

// PWM TEST HARNESS
//#define PWM_TEST
#ifdef PWM_TEST
// SUCCESS - 
 // PWM_0 = 10 DC
 // PWM_1 = 20 DC
 // PWM_2 = 30 DC
 // PWM_3 = 50 DC
 // PWM_4 = 75 DC
 // PWM_5 = 100 DC
 // ALL CHANNELS @ 5 khz

#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <timers.h>
#include <pwm.h>

int main(void) {
    BOARD_Init();
    TIMER_Init();
    if (PWM_Init() == SUCCESS) {
        printf("pwm initialized\r\n");

        // add all pins and set duty cycles
        if (PWM_SetDutyCycle(PWM_0, 10) == ERROR) {
            printf("pwm set duty cycle error\r\n");
            return -1;
        }
        if (PWM_SetDutyCycle(PWM_1, 20) == ERROR) {
            printf("pwm set duty cycle error\r\n");
            return -1;
        }
        if (PWM_SetDutyCycle(PWM_2, 30) == ERROR) {
            printf("pwm set duty cycle error\r\n");
            return -1;
        }
        if (PWM_SetDutyCycle(PWM_3, 50) == ERROR) {
            printf("pwm set duty cycle error\r\n");
            return -1;
        }
        if (PWM_SetDutyCycle(PWM_4, 75) == ERROR) {
            printf("pwm set duty cycle error\r\n");
            return -1;
        }
        if (PWM_SetDutyCycle(PWM_5, 100) == ERROR) {
            printf("pwm set duty cycle error\r\n");
            return -1;
        }
        PWM_SetFrequency(5000);
    }

    while (TRUE);
}

#endif
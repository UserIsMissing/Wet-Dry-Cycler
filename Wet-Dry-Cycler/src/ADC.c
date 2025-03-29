/**
 * @file    ADC.c
 *
 * Configure and read from ADC channels using the UCSC Nucleo I/O shield.
 *
 * @author  HARE Lab
 * @author  jLab
 * @author  Adam Korycki
 *
 * @date    16 Sep 2023
 *
 * @TODO    Switch to an interrupt-driven ADC framework that uses DMA to read/
 *          store data from each of the 7 channels when triggered. This special
 *          approach is required since the ADC SFRs are not labeled by channel,
 *          i.e. otherwise we cannot tell which ADC value was read by which
 *          ADC channel (values are mixed up).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ADC.h"


/*  PROTOTYPES  */
static int8_t ADC_ConfigPins(void);
static int8_t ADC_ConfigClks(void);


/*  MODULE-LEVEL DEFINITIONS, MACROS    */
static int8_t initStatus = FALSE;


/*  FUNCTIONS   */
/** ADC_ConfigPins()
 *
 * Configure pins for use with the ADC.
 * NOTE: This only configures the relevant pins to ADC1; it does not initialize
 * the ADC interrupts.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t ADC_ConfigPins(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    return SUCCESS;
}

/** ADC_ConfigClks()
 *
 * Configure clocks for use with the ADC.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
static int8_t ADC_ConfigClks(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    return SUCCESS;
}

/** ADC_Start()
 *
 * Start ADC convesions.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t ADC_Start(void)
{
    HAL_ADC_Start(&hadc1);

    return SUCCESS;
}

/** ADC_End()
 *
 * Disables the A/D subsystem and releases the pins used.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t ADC_End(void)
{
    HAL_ADC_Stop(&hadc1);
	HAL_ADC_DeInit(&hadc1);

    return SUCCESS;
}

/** ADC_Read(channel)
 *
 * Selects ADC channel and returns 12-bit reading.
 *
 * @param   channel (uint32_t)  Select ADC channel:
 *                                  [ADC_0, ADC_1, ..., ADC_5, POT]
 * @return          (uint16_t)  12-bit ADC reading.
 */
uint16_t ADC_Read(uint32_t channel)
{
	// Select channel and sampling time.
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	
	// Start ADC Conversion, wait for conversion and return value.
    ADC_Start();
	HAL_ADC_PollForConversion(&hadc1, 1);

	return HAL_ADC_GetValue(&hadc1);
}

/** ADC_Init()
 *
 * Initializes the ADC subsystem with an interrupt; selects ADC Pin4 by default.
 * 
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t ADC_Init(void)
{
	if (initStatus == FALSE)
    {
        ADC_ConfigPins();
        ADC_ConfigClks();

        /**
         * Configure the global features of the ADC (clock, resolution, data
         * alignment and number of conversions).
         */
        hadc1.Instance = ADC1;
        hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
        hadc1.Init.Resolution = ADC_RESOLUTION_12B;
        hadc1.Init.ScanConvMode = DISABLE;
        hadc1.Init.ContinuousConvMode = ENABLE;
        hadc1.Init.DiscontinuousConvMode = DISABLE;
        hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
        hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
        hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
		hadc1.Init.NbrOfConversion = 1;
        hadc1.Init.DMAContinuousRequests = DISABLE;
        hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

        if (HAL_ADC_Init(&hadc1) != HAL_OK)
        {
          return ERROR;
        }

		// By default, select onboard potentiometer.
        ADC_ChannelConfTypeDef sConfig = {0};
		sConfig.Channel = POT;
		sConfig.Rank = 1;
		sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
        {
			return ERROR;
		}

        // Start ADC.
        ADC_Start();

        // Complete initialization.
        initStatus = TRUE;
    }

	return SUCCESS;
}


/** ADC_TEST
 * 
 * Uncomment the below "#define" to run the ADC_TEST.
 *
 * SUCCESS - Printed analog values for each channel are correct.
 */
//#define ADC_TEST
#ifdef ADC_TEST

#include <stdio.h>
#include <Board.h>
#include <ADC.h>


int main(void)
{
	BOARD_Init();
	char ret = ADC_Init();

	if (ret != ERROR)
    {
		while (TRUE)
        {
			printf("POT   = %d\r\n", ADC_Read(POT));
			printf("ADC_0 = %d\r\n", ADC_Read(ADC_0));
			printf("ADC_1 = %d\r\n", ADC_Read(ADC_1));
			printf("ADC_2 = %d\r\n", ADC_Read(ADC_2));
			printf("ADC_3 = %d\r\n", ADC_Read(ADC_3));
			printf("ADC_4 = %d\r\n", ADC_Read(ADC_4));
			printf("ADC_5 = %d\r\n\r\n", ADC_Read(ADC_5));
			HAL_Delay(500);
		}
	} else {
		printf("ADC init error");
	}
}


#endif  /*  ADC_TEST    */

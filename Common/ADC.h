/**
 * @file    Adc.h
 *
 * Configure and read from ADC channels using the UCSC Nucleo I/O shield.
 *
 * @author  Adam Korycki
 * @author  HARE Lab
 * @author  jLab
 *
 * @date    16 Sep 2023
 *
 * @TODO    Switch to an interrupt-driven ADC framework that uses DMA to read/
 *          store data from each of the 7 channels when triggered. This special
 *          approach is required since the ADC SFRs are not labeled by channel,
 *          i.e. otherwise we cannot tell which ADC value was read by which
 *          ADC channel (values are mixed up).
 */
#ifndef ADC_H
#define	ADC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"


/*  MODULE-LEVEL VARIABLES, MACROS  */
#define ADC_0               ADC_CHANNEL_0
#define ADC_1               ADC_CHANNEL_1
#define POT                 ADC_CHANNEL_4   // Onboard potentiometer.
#define ADC_2               ADC_CHANNEL_10
#define ADC_3               ADC_CHANNEL_11
#define ADC_4               ADC_CHANNEL_12
#define ADC_5               ADC_CHANNEL_13
#define ADC_NUM_CHANNELS    7
#define ADC_MIN             0
#define ADC_MAX             4095

#ifndef FALSE
#define FALSE ((int8_t) 0)
#endif  /*  FALSE   */
#ifndef TRUE
#define TRUE ((int8_t) 1)
#endif  /*  TRUE    */
#ifndef ERROR
#define ERROR ((int8_t) -1)
#endif  /*  ERROR   */
#ifndef SUCCESS
#define SUCCESS ((int8_t) 1)
#endif  /*  SUCCESS */

ADC_HandleTypeDef hadc1;


/*  PROTOTYPES  */
/** ADC_Start()
 *
 * Start ADC conversions for polling.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t ADC_Start(void);

/** ADC_End()
 *
 * Disables the A/D subsystem and releases the pins used.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t ADC_End(void);

/** ADC_Read(channel)
 *
 * Selects ADC channel and returns 12-bit reading.
 *
 * @param   channel (uint32_t)  Select ADC channel
 *                                  (ADC_0, ADC_1, ..., ADC_5, POT)
 * @return          (uint16_t)  12-bit ADC reading.
 */
uint16_t ADC_Read(uint32_t channel);

/** ADC_Init()
 *
 * Initializes the ADC subsystem with an interrupt; selects ADC Pin4 by default.
 * 
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t ADC_Init(void);


#endif  /*  ADC_H   */

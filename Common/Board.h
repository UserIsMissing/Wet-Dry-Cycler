/**
 * @file    BOARD.h
 * 
 * Generic driver set up and clean up for functional testing for the UCSC Nucleo
 * I/O Shield connected to a Nucleo STM32F411RE development kit.
 *
 * @author  HARE Lab
 * @author  Adam Korycki
 *
 * @date    18 Dec 2024
 */
#ifndef BOARD_H
#define	BOARD_H

#include <stdint.h>
#include "stm32f4xx_hal.h"


/*  MODULE-LEVEL DEFINITIONS, MACROS    */
// Boolean defines for TRUE, FALSE, SUCCESS and ERROR.
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

// Define standard error codes.
enum {
  SIZE_ERROR = -1,
  STANDARD_ERROR
};

// Defined here for legacy support. <-- TODO: Check if these are used.
#define B1_Pin GPIO_PIN_13 // onboard blue push button
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2 // USB-UART (USART2)
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA


/*  PROTOTYPES  */
/** BOARD_Init()
 *
 * 1. Initialize the HAL framework.
 * 2. Configure the clocks and pins used by our Nucleo kit's setup for serial
 * communications, onboard LED statuses, and resetting with the Big Blue Button.
 * 3. Initialize LEDs.
 *
 * @return  (int8_t)    [SUCCESS, ERROR]
 */
int8_t BOARD_Init(void);

/** BOARD_End()
 *
 * Shuts down all peripherals; turns all pins into input.
 *
 * @return  initStatus  (int8_t)    [TRUE, FALSE]
 *
 * @todo    Set all ports to be digital inputs.
 */
int8_t BOARD_End(void);

/** Error_Handler()
 * 
 * Enter a state of blinking the status LED on-board the Nucleo, indefinitely.
 */
void Error_Handler(void);


#endif  /*  BOARD_H */

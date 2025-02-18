/*
 * File:   uart.h
 * Author: Adam Korycki
 *
 * Library functions for two seperate UART interafaces (UART1 & UART6)
 * 
 * Created on November 2, 2023
 */

#ifndef UART_H
#define	UART_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

/**
 * @Function Uart1_Init(Rate)
 * @param Rate - baudrate (must be between 9600 and 115200)
 * @return SUCCESS or ERROR
 * @brief  Initializes uart1 interface at specified baudrate
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart1_Init(int Rate);

/**
 * @Function Uart1_rx(uint8_t* data, uint8_t size)
 * @param data - pointer to received buffer
 * @param size - number of byte read from buffer
 * @return SUCCESS or ERROR
 * @brief  Reads data from the rx buffer for uart1.
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart1_rx(uint8_t* data, uint16_t size);

/**
 * @Function Uart1_tx(uint8_t* data)
 * @param data - byte buffer to transmit
 * @param size - number of byte send from buffer
 * @return SUCCESS or ERROR
 * @brief  Transmits <size> number of bytes on uart1
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart1_tx(uint8_t* data, uint16_t size);

/**
 * @Function Uart6_Init(Rate)
 * @param Rate - baudrate (must be between 9600 and 115200)
 * @return SUCCESS or ERROR
 * @brief  Initializes uart6 interface at specified baudrate
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart6_Init(int Rate);    

/**
 * @Function Uart6_rx(uint8_t* data, uint8_t size)
 * @param data - pointer to received buffer
 * @param size - number of byte read from buffer
 * @return SUCCESS or ERROR
 * @brief  Reads data from the rx buffer for uart6.
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart6_rx(uint8_t* data, uint16_t size);

/**
 * @Function Uart6_tx(uint8_t* data)
 * @param data - byte buffer to transmit
 * @param size - number of bytes to send from buffer
 * @return SUCCESS or ERROR
 * @brief  Transmits <size> number of bytes on uart6
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart6_tx(uint8_t* data, uint16_t size);

#endif
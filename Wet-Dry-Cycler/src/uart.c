/*
 * File:   uart.c
 * Author: Adam Korycki
 *
 * Library for two seperate UART interafaces (UART1 & UART6)
 * 
 * Created on November 2, 2023
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <uart.h>

// Boolean defines for TRUE, FALSE, SUCCESS and ERROR
#ifndef FALSE
#define FALSE ((int8_t) 0)
#define TRUE ((int8_t) 1)
#endif
#ifndef ERROR
#define ERROR ((int8_t) -1)
#define SUCCESS ((int8_t) 1)
#endif

static uint8_t init_status_uart1 = FALSE;
static uint8_t init_status_uart6 = FALSE;

/**
 * @Function Uart1_Init(Rate)
 * @param Rate - baudrate (must be between 9600 and 115200)
 * @return SUCCESS or ERROR
 * @brief  Initializes uart1 interface at specified baudrate
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart1_Init(int Rate) {
    if ((Rate < 9600) ||(Rate > 115200)) { // check if baudrate is within limits
        printf("baudrate must be between 9600 and 115200\r\n");
        return ERROR;
    }
    if (init_status_uart1 == FALSE) {
        huart1.Instance = USART1;
        huart1.Init.BaudRate = Rate;
        huart1.Init.WordLength = UART_WORDLENGTH_8B;
        huart1.Init.StopBits = UART_STOPBITS_1;
        huart1.Init.Parity = UART_PARITY_NONE;
        huart1.Init.Mode = UART_MODE_TX_RX;
        huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        huart1.Init.OverSampling = UART_OVERSAMPLING_16;
        if (HAL_UART_Init(&huart1) != HAL_OK)
        {
            return ERROR;
        }
        init_status_uart1 = TRUE;
    }
    return SUCCESS;
}

/**
 * @Function Uart1_rx(uint8_t* data, uint8_t size)
 * @param data - pointer to received buffer
 * @param size - number of bytes to read from buffer
 * @return SUCCESS or ERROR
 * @brief  Reads data from the rx buffer for uart1.
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart1_rx(uint8_t* data, uint16_t size) {
    if (init_status_uart1 == FALSE) {
        printf("Uart1 not yet initialized\r\n");
        return ERROR;
    }
    HAL_UART_Receive_IT(&huart1, data, size);
    return SUCCESS;
}

/**
 * @Function Uart1_tx(uint8_t* data)
 * @param data - byte buffer to transmit
 * @param size - number of bytes to send from buffer
 * @return SUCCESS or ERROR
 * @brief  Transmits <size> number of bytes on uart1
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart1_tx(uint8_t* data, uint16_t size) {
    if (init_status_uart1 == FALSE) {
        printf("Uart1 not yet initialized\r\n");
        return ERROR;
    }
    HAL_UART_Transmit_IT(&huart1, data, size);
    return SUCCESS;
}

/**
 * @Function Uart6_Init(Rate)
 * @param Rate - baudrate (must be between 9600 and 115200)
 * @return SUCCESS or ERROR
 * @brief  Initializes uart6 interface at specified baudrate
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart6_Init(int Rate) {
    if ((Rate < 9600) ||(Rate > 115200)) { // check if baudrate is within limits
        printf("baudrate must be between 9600 and 115200\r\n");
        return ERROR;
    }
    if (init_status_uart6 == FALSE) {
        huart6.Instance = USART6;
        huart6.Init.BaudRate = Rate;
        huart6.Init.WordLength = UART_WORDLENGTH_8B;
        huart6.Init.StopBits = UART_STOPBITS_1;
        huart6.Init.Parity = UART_PARITY_NONE;
        huart6.Init.Mode = UART_MODE_TX_RX;
        huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        huart6.Init.OverSampling = UART_OVERSAMPLING_16;
        if (HAL_UART_Init(&huart6) != HAL_OK)
        {
            return ERROR;
        }
        init_status_uart6 = TRUE;
    }
    return SUCCESS;
}

/**
 * @Function Uart6_rx(uint8_t* data, uint8_t size)
 * @param data - pointer to received buffer
 * @param size - number of bytes to read from buffer
 * @return SUCCESS or ERROR
 * @brief  Reads data from the rx buffer for uart6.
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart6_rx(uint8_t* data, uint16_t size) {
    if (init_status_uart6 == FALSE) {
        printf("Uart1 not yet initialized\r\n");
        return ERROR;
    }
    HAL_UART_Receive_IT(&huart6, data, size);
    return SUCCESS;
}

/**
 * @Function Uart6_tx(uint8_t* data)
 * @param data - byte buffer to transmit
 * @param size - number of bytes to send from buffer
 * @return SUCCESS or ERROR
 * @brief  Transmits <size> number of bytes on uart6
 * @author Adam Korycki, 2023.11.02 */
int8_t Uart6_tx(uint8_t* data, uint16_t size) {
    if (init_status_uart6 == FALSE) {
        printf("Uart6 not yet initialized\r\n");
        return ERROR;
    }
    HAL_UART_Transmit_IT(&huart6, data, size);
    return SUCCESS;
}

//#define UART_TEST
#ifdef UART_TEST //UART TEST HARNESS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Board.h>
#include <timers.h>
#include <uart.h>

/* connect UART1 and UART6 together, 
 * if a classic chant is printed to std_out over uart2 then all is well */
int main(void) {
    BOARD_Init();

    if ((Uart1_Init(115200) == SUCCESS) && (Uart6_Init(115200) == SUCCESS)) {
        printf("uarts initialized\r\n");
        char rx[80];
        char tx[80];
        uint32_t beer = 0;

       while (TRUE) {
        sprintf(tx, "%d bottles of beer on the wall\r\n", beer);
        Uart6_tx(tx, strlen(tx));
        Uart1_rx(rx, strlen(tx));
        printf("%s", rx);
        beer++;
        HAL_Delay(100);
       }
    }
    else {
        printf("UART init error\r\n");
    }
    printf("done\r\n");
    while(TRUE);
}
#endif

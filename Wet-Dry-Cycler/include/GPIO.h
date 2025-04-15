#ifndef GPIO_H
#define GPIO_H

//Created a GPIO library to toggle LEDS


#include "stm32f4xx_hal.h"
// REHYDRATION MOTOR
#define GPIO_C0 PIN_0
#define GPIO_C1 PIN_1
#define GPIO_C2 PIN_2
#define GPIO_C3 PIN_3
#define GPIO_C13 PIN_13
#define GPIO_C14 PIN_14
#define GPIO_C15 PIN_15
// MOVEMENT Motor
#define GPIO_B4 PIN_27
#define GPIO_B5 PIN_29
#define GPIO_B3 PIN_31
#define GPIO_A1 PIN_30
#define GPIO_A4 PIN_32
#define GPIO_B0 PIN_34
// MOVEMENT BUMPERS
#define GPIO_A2 PIN_35
#define GPIO_A3 PIN_37

#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

/**
 * @brief Enumerate each pin you want to manage in this driver.
 */
typedef enum {
    GPIO_C0,
    GPIO_C1,
    GPIO_C2,
    GPIO_C3,
    GPIO_C13,
    GPIO_C14,
    GPIO_C15,
    GPIO_B4,
    GPIO_B5,
    GPIO_B3,
    GPIO_A1,
    GPIO_A4,
    GPIO_B0,
    GPIO_A2,
    GPIO_A3,
    GPIO_2_NUM_PINS // keep this last as a “count” of pins
} Gpio2Pin_t;

/**
 * @brief Initializes all pins we want to use as GPIO.
 */
void GPIO_Init(void);

/**
 * @brief Write a High or Low state to the specified GPIO pin.
 *
 * @param pin   The pin from Gpio2Pin_t
 * @param state Either HIGH or LOW
 */
void GPIO_WritePin(Gpio2Pin_t pin, GPIO_PinState state);

/**
 * @brief Toggle the specified GPIO pin.
 *
 * @param pin   The pin from Gpio2Pin_t
 */
void GPIO_TogglePin(Gpio2Pin_t pin);

/**
 * @brief Read the input state of the specified GPIO pin.
 *
 * @param pin   The pin from Gpio2Pin_t
 * @return      GPIO_PinState (GPIO_PIN_SET or GPIO_PIN_RESET)
 */
GPIO_PinState GPIO_ReadPin(Gpio2Pin_t pin);

#endif // GPIO_H

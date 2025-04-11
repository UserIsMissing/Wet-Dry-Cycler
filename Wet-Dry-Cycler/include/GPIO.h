#ifndef GPIO_H
#define GPIO_H

//Created a GPIO library to toggle LEDS


#include "stm32f4xx_hal.h"

#define GPIO_0 PIN_0
#define GPIO_1 PIN_1
#define GPIO_2 PIN_2
#define GPIO_3 PIN_3
#define GPIO_13 PIN_13
#define GPIO_14 PIN_14
#define GPIO_15 PIN_15

#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

/**
 * @brief Enumerate each pin you want to manage in this driver.
 */
typedef enum {
    GPIO_0,
    GPIO_1,
    GPIO_2,
    GPIO_3,
    GPIO_13,
    GPIO_14,
    GPIO_15,
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

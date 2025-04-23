#ifndef GPIO_H
#define GPIO_H

//Created a GPIO library to toggle LEDS


#include "stm32f4xx_hal.h"
// REHYDRATION MOTOR
#define GPIO_C0 PIN_C0
#define GPIO_C1 PIN_C1
#define GPIO_C3 PIN_C3
#define GPIO_C10 PIN_C10
#define GPIO_C11 PIN_C11
#define GPIO_C12 PIN_C12
#define GPIO_A15 PIN_A15
// // MOVEMENT Motor
#define GPIO_B4 PIN_B4
#define GPIO_B5 PIN_B5
#define GPIO_A7 PIN_A7
#define GPIO_A1 PIN_A1
#define GPIO_A4 PIN_A4
#define GPIO_B0 PIN_B0
#define GPIO_C2 PIN_C2

// MOVEMENT BUMPERS
#define GPIO_A5 PIN_A5
#define GPIO_A6 PIN_A6
#define GPIO_B8 PIN_B8

// MIXING PINS
#define GPIO_C8 PIN_C8
#define GPIO_C9 PIN_C9
#define GPIO_B2 PIN_B2

// HEATING PAD
#define GPIO_B1 PIN_B1

#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

/**
 * @brief Enumerate each pin you want to manage in this driver.
 */
typedef enum {
    // REHYDRATION Motor
    GPIO_C0,
    GPIO_C1,
    GPIO_C3,
    GPIO_C10,
    GPIO_C11,
    GPIO_C12,
    GPIO_A15,

    // MOVEMENT Motor
    GPIO_B4,
    GPIO_B5,
    GPIO_A7,
    GPIO_A1,
    GPIO_A4,
    GPIO_B0,
    GPIO_C2,

    // MOVEMENT Bumpers
    GPIO_A5,
    GPIO_A6,
    GPIO_B8,

    // MIXING Pins
    GPIO_C8,
    GPIO_C9,
    GPIO_B2,

    // HEATING 
    GPIO_B1,

        
    
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

#include "GPIO.h"

/**
 * @brief Internal structure to map each Gpio2Pin_t enum to (port, pin).
 */
static const struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} gpioPinTable[GPIO_2_NUM_PINS] = {
    // REHYDRATION Motor
    [PIN_C0]  = {GPIOC, GPIO_PIN_0},
    [PIN_C1]  = {GPIOC, GPIO_PIN_1},
    [PIN_C2]  = {GPIOC, GPIO_PIN_2},
    [PIN_C3]  = {GPIOC, GPIO_PIN_3},
    [PIN_C13]  = {GPIOC, GPIO_PIN_13},
    [PIN_C14]  = {GPIOC, GPIO_PIN_14},
    [PIN_C15]  = {GPIOC, GPIO_PIN_15},
    // MOVEMENT Motor
    [PIN_B4]  = {GPIOB, GPIO_PIN_4},
    [PIN_B5]  = {GPIOB, GPIO_PIN_5},
    [PIN_A7]  = {GPIOA, GPIO_PIN_7},
    [PIN_A1]  = {GPIOA, GPIO_PIN_1},
    [PIN_A4]  = {GPIOA, GPIO_PIN_4},
    [PIN_B0]  = {GPIOB, GPIO_PIN_0},
    // MOVEMENT Bumpers
    [PIN_A5]  = {GPIOA, GPIO_PIN_5},
    [PIN_A6]  = {GPIOA, GPIO_PIN_6},
    [PIN_B8]  = {GPIOB, GPIO_PIN_8},
};

/**
 * @brief Initializes all the pins (PA8, PA9, PA10, PA11, PB6, PB8) as general 
 *        purpose outputs, push-pull, no pull-ups. 
 *
 *        If you need input pins or pull-ups, adapt the config below.
 */
void GPIO_Init(void)
{
    // 1. Enable clocks for GPIOA and GPIOB if not already enabled.
    //    The calls below are safe even if the clocks are already on.
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 2. Configure pins in a loop
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;    // push-pull output
    GPIO_InitStruct.Pull  = GPIO_NOPULL;            // no pull-up/down
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;    // speed as needed

    for (int i = 0; i < GPIO_2_NUM_PINS; i++) {
        GPIO_InitStruct.Pin = gpioPinTable[i].pin;
        HAL_GPIO_Init(gpioPinTable[i].port, &GPIO_InitStruct);
        // Optional: set each pin LOW initially
        HAL_GPIO_WritePin(gpioPinTable[i].port, gpioPinTable[i].pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Write a logic state (SET or RESET) to the specified pin.
 */
void GPIO_WritePin(Gpio2Pin_t pin, GPIO_PinState state)
{
    HAL_GPIO_WritePin(gpioPinTable[pin].port, gpioPinTable[pin].pin, state);
}

/**
 * @brief Toggle the specified pin (if it’s an output).
 */
void GPIO_TogglePin(Gpio2Pin_t pin)
{
    HAL_GPIO_TogglePin(gpioPinTable[pin].port, gpioPinTable[pin].pin);
}

/**
 * @brief Read the input or output state of the specified pin.
 *
 * @return GPIO_PinState (GPIO_PIN_SET or GPIO_PIN_RESET)
 */
GPIO_PinState GPIO_ReadPin(Gpio2Pin_t pin)
{
    return HAL_GPIO_ReadPin(gpioPinTable[pin].port, gpioPinTable[pin].pin);
}

/*
 * File:   Buttons.h
 * Author: Adam Korycki
 *
 * Created on November 15, 2023
 */
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/**
 * @function BUTTONS_Init(void)
 * @param None
 * @return None
 * @brief Initializes GPIO inputs for the four push buttons on the IO shield
 * @author Adam Korycki, 2023.11.15 */
void BUTTONS_Init(void);

/**
 * @function buttons_state(void)
 * @param None
 * @return byte where the lower 4-bits represent the state of the buttons (lsb -> button_0)
 * @brief returns state of push buttons
 * @author Adam Korycki, 2023.11.15 */
uint8_t buttons_state(void);

#endif
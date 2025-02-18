/*
 * File:   leds.h
 * Author: Adam Korycki
 *
 * Created on November 15, 2023
 */
#ifndef LEDS_H
#define LEDS_H

/**
 * @function LEDS_Init(void)
 * @param None
 * @return None
 * @brief Initializes GPIO outputs for the eight leds on the IO shield
 * @author Adam Korycki, 2023.11.15 */
void LEDS_Init(void);

/**
 * @function set_leds(uint8_t leds)
 * @param leds - byte where each bit represents the state of the leds (lsb -> LED1)
 * @return None
 * @brief sets leds according to byte parameter
 * @author Adam Korycki, 2023.11.15 */
void set_leds(uint8_t leds);

#endif
/**
 * @file    MIXING.h
 * @brief   Motor control module using GPIO for digital ON/OFF.
 *
 * This module provides initialization and control functions for
 * turning mixing motors ON and OFF via GPIO pins. Motors can be
 * controlled individually or all at once.
 *
 * Compatible with ESP32 and Arduino platforms.
 *
 * @author  Rafael Delwart
 * @date    25 Feb 2025 adapted for ESP 13 May 2025
 */

 #ifndef MIXING_H
 #define MIXING_H
 
 #include <Arduino.h>
 
 // === API ===
 
 /**
  * @brief Initializes motor GPIO pins.
  *
  * Sets all motor pins as OUTPUT and initializes them to LOW (OFF).
  * Call this once in setup().
  */
 void MIXING_Init();
 
 /**
  * @brief Turns ON a motor connected to the specified GPIO pin.
  *
  * @param pin GPIO pin number to turn ON (HIGH).
  */
 void MIXING_Motor_OnPin(uint8_t pin);
 
 /**
  * @brief Turns OFF a motor connected to the specified GPIO pin.
  *
  * @param pin GPIO pin number to turn OFF (LOW).
  */
 void MIXING_Motor_OffPin(uint8_t pin);
 
 /**
  * @brief Turns ON all motors.
  *
  * Drives all defined motor GPIO pins HIGH.
  */
 void MIXING_AllMotors_On();
 
 /**
  * @brief Turns OFF all motors.
  *
  * Drives all defined motor GPIO pins LOW.
  */
 void MIXING_AllMotors_Off();
 
 #endif  // MIXING_H
 
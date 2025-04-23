/**
 * @file    Rehydration.h
 *
 * @brief   Header for Rehydration control module
 *
 * Declares functions to initialize and operate a syringe pump stepper motor
 * via DRV8825 driver, used for dispensing precise fluid volumes.
 *
 * @author  Rafael Delwart
 * @date    20 Apr 2025
 */

#pragma once
#include "main.h"

 #ifndef REHYDRATION_H
 #define REHYDRATION_H
 
 /**
  * @brief Initializes the syringe pump stepper motor
  */
 void Rehydration_Init(void);
 
 /**
  * @brief Dispenses specified microliters by pushing plunger forward
  * @param uL Amount of liquid in microliters to push
  */
 void Rehydration_Push(uint32_t uL);
 
 /**
  * @brief Pulls back plunger to draw fluid or prime
  * @param uL Volume in microliters to pull
  */
 void Rehydration_Pull(uint32_t uL);
 
 /**
  * @brief Disables motor driver to save power and reduce heat
  */
 void Rehydration_Stop(void);
 
 #endif // REHYDRATION_H
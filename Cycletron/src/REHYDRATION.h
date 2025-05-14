/**
 * @file    REHYDRATION.h
 * @brief   Syringe pump control interface for RNA fluid rehydration
 *
 * This header declares the control functions for a stepper motor
 * used to precisely dispense and retract fluid via a syringe pump.
 * It uses a DRV8825 stepper motor driver to move the plunger and
 * convert microliter requests into accurate step commands.
 *
 * Designed for use in wet-dry cycling systems for RNA research.
 *
 * Author: Rafael Delwart
 * Date:   20 Apr 2025
 */

 #ifndef REHYDRATION_H
 #define REHYDRATION_H
 
 #include <Arduino.h>
 #include "DRV8825.h"
 
 /**
  * @brief Initializes the syringe pump stepper motor.
  *
  * This sets up the DRV8825 driver pins, applies the default microstepping mode,
  * and prints calibration data including the volume of fluid dispensed per motor step.
  * Call this once during system setup.
  */
 void Rehydration_Init();
 
 /**
  * @brief Dispenses a specific volume of liquid in microliters by pushing the plunger forward.
  *
  * Converts the desired microliters to motor steps based on syringe and leadscrew dimensions,
  * and moves the motor forward (push) at a controlled speed to ensure accurate fluid delivery.
  *
  * @param uL Volume of fluid to dispense, in microliters.
  */
 void Rehydration_Push(uint32_t uL);
 
 /**
  * @brief Retracts the syringe plunger to draw fluid or prime the system.
  *
  * Similar to Rehydration_Push(), but moves in reverse to create suction.
  * Useful for loading liquid into the syringe or preparing it for operation.
  *
  * @param uL Volume of fluid to draw back, in microliters.
  */
 void Rehydration_Pull(uint32_t uL);
 
 /**
  * @brief Disables the stepper motor to stop motion and reduce power consumption.
  *
  * This function cuts power to the DRV8825 driver, halting the motor and preventing
  * unnecessary heating or energy drain. It should be called after movement is complete.
  */
 void Rehydration_Stop();
 
 #endif  // REHYDRATION_H
 
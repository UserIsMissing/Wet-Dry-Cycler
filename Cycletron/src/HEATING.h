/**
 * @file    HEATING.h
 * @brief   Silicon Heating Pad control module header for ESP32
 *
 * This module implements simple bang-bang temperature control
 * of a heating pad using an NTC thermistor and voltage divider.
 * Raw ADC values are converted to voltage, resistance, and temperature
 * using the BETA model. Moving average filters are applied to smooth noise.
 *
 * @author  Rafael Delwart
 * @date    20 Feb 2025 (ESP32 update: May 2025)
 */

#ifndef HEATING_H
#define HEATING_H

#include <Arduino.h>


#define HEATING_GPIO 5         // GPIO to control heater

/**
 * @brief Initializes GPIO for heating pad control and sets up ADC.
 *
 * Configures the GPIO pin used to control the heater and sets it LOW (off).
 * Prepares internal buffer states for moving average filtering.
 */
void HEATING_Init(void);

/**
 * @brief Reads a single raw MV value from the configured thermistor pin.
 *
 * This function performs a direct analog read of the voltage divider output.
 *
 * @return Raw MV value (range: 0-3300 mV)
 *         or 0 if the read fails
 *
 */
int HEATING_Measure_Raw_MV(void);

/**
 * @brief Returns the moving average of raw MV readings.
 *
 * Smooths out rapid fluctuations by averaging over a window of samples.
 *
 *  @return Average MV value (range: 0-3300 mV)
 *         or 0 if the read fails
 */
float HEATING_Measure_AVG_MV(void);

/*

/**
 * @brief Calculates thermistor resistance based on the voltage divider.
 *
 * Uses the known series resistor and measured voltage to solve for thermistor resistance.
 *
 * @return Thermistor resistance in Ohms
 */
float HEATING_Measure_Resistance(void);

/**
 * @brief Converts thermistor resistance to temperature using the BETA model.
 *
 * Applies the simplified Steinhart-Hart BETA equation to determine temperature.
 *
 * @return Temperature in degrees Celsius
 */
float HEATING_Measure_Temp(void);

/**
 * @brief Returns a moving average of recent temperature readings.
 *
 * Reduces measurement noise and improves stability for control decisions.
 *
 * @return Averaged temperature in degrees Celsius
 */
float HEATING_Measure_Temp_Avg(void);

/**
 * @brief Bang-bang control: toggles heater based on target temperature.
 *
 * If the measured temperature is below the setpoint, turns the heater ON.
 * If the measured temperature is at or above the setpoint, turns it OFF.
 *
 * @param setpointCelsius Desired target temperature in °C
 */
void HEATING_Set_Temp(int setpointCelsius);

/**
 * @brief Turns off temperature controller.
 *
 */
void HEATING_Off();

#endif // HEATING_H

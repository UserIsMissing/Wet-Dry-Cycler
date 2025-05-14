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
 
 /**
  * @brief Initializes GPIO for heating pad control and sets up ADC.
  *
  * Configures the GPIO pin used to control the heater and sets it LOW (off).
  * Prepares internal buffer states for moving average filtering.
  */
 void HEATING_Init(void);
 
 /**
  * @brief Reads a single raw ADC value from the configured thermistor pin.
  *
  * This function performs a direct analog read of the voltage divider output.
  *
  * @return Raw ADC value (range: 0–4095)
  */
 int HEATING_Measure_Raw_ADC(void);
 
 /**
  * @brief Returns the moving average of raw ADC readings.
  *
  * Smooths out rapid fluctuations by averaging over a window of samples.
  *
  * @return Averaged raw ADC value
  */
 int HEATING_Measure_Raw_ADC_Avg(void);
 
 /**
  * @brief Converts the averaged ADC value to voltage.
  *
  * Applies standard ADC-to-voltage conversion using VREF and resolution.
  *
  * @return Measured voltage in volts (0.0–3.3V)
  */
 float HEATING_Measure_Voltage(void);
 
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
 
 #endif  // HEATING_H
 
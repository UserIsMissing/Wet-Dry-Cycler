/**
 * @file    Heating.h
 *
 * @brief   Silicon Heating Pad control module header for ESP32
 *
 * @author  Rafael Delwart
 * @date    20 Feb 2025 (ESP32 update: May 2025)
 *
 * @details This module implements simple bang-bang temperature control
 *          of a heating pad using an NTC thermistor and voltage divider.
 *          Raw ADC values are converted to voltage, resistance, and temperature
 *          using the BETA model. Moving average filters are applied to smooth noise.
 */

 #ifndef HEATING_H
 #define HEATING_H
 
 // === FUNCTION DECLARATIONS ========================================
 
 /**
  * @brief Initializes GPIO for heating pad control.
  *        Also prepares internal state (buffers, flags).
  */
 void HEATING_Init(void);
 
 /**
  * @brief Reads a single raw ADC value from the configured thermistor pin.
  *        This calls an external ADC function provided in main.c.
  *
  * @return Raw ADC value (0–4095)
  */
 int HEATING_Measure_Raw_ADC(void);
 
 /**
  * @brief Calculates the moving average of raw ADC readings
  *        for smoother voltage computation.
  *
  * @return Averaged ADC value
  */
 int HEATING_Measure_Raw_ADC_Avg(void);
 
 /**
  * @brief Converts averaged ADC value to voltage across the thermistor.
  *
  * @return Voltage in volts
  */
 float HEATING_Measure_Voltage(void);
 
 /**
  * @brief Calculates thermistor resistance using the voltage divider formula.
  *
  * @return Resistance in Ohms
  */
 float HEATING_Measure_Resistance(void);
 
 /**
  * @brief Computes current heating pad temperature in Celsius
  *        using the BETA model and thermistor resistance.
  *
  * @return Temperature in °C
  */
 float HEATING_Measure_Temp(void);
 
 /**
  * @brief Returns a moving average of the most recent temperature readings
  *        for improved stability.
  *
  * @return Averaged temperature in °C
  */
 float HEATING_Measure_Temp_Avg(void);
 
 /**
  * @brief Implements simple bang-bang control:
  *        Turns heater ON if temperature is below target,
  *        and OFF if above.
  *
  * @param Temp Desired temperature in °C
  */
 void HEATING_Set_Temp(int Temp);
 
 #endif /* HEATING_H */
 
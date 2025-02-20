/**
 * @file    Heating.h
 *
 * Silicon Heating Pad control module 
 *
 * @author  Rafael Delwart
 * @date    20 Feb 2025
 *
 * @detail  This module uses the external interrupt peripheral to detect touch
 *          inputs. The peripheral is configured to generate an interrupt every
 *          rising edge of pin PB5 (ENC_B) which means that the difference of
 *          two interrupts gives you the period of the signal. Use the timers.h
 *          library for timing operations.
 * 
 *         
 *          
 */

#ifndef HEATING_H
#define HEATING_H

#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <I2C.h>
#include <math.h>
#include <timers.h>
#include <ADC.h>



/** HEATING_Init()
 *
 * This function initializes the module for use. Initialization is done by initializing the ADC then
 * opening and configuring the ADC pin used to measure the voltage from the voltage divider.
 * 
 */
void HEATING_Init(void);

/** HEATING_Measure_Raw_ADC(void)
 *
 * Returns the measured raw ADC.
 *
 * @return  (int)  [raw ADC]
 */
int HEATING_Measure_Raw_ADC(void);



/** HEATING_Measure_Voltage(void)
 *
 * Returns the measured voltage from the voltage divider used to determine temperature.
 *
 * @return  (float)  [Volts]
 */
float HEATING_Measure_Voltage(void);




/** HEATING_Measure_Resistance(void)
 *
 * Returns the current resistance of the thermistor pad in Ohms, calculated based on voltage readings.
 *
 * @return  (float)  [Ohms]
 */
float HEATING_Measure_Resistance(void);



/** HEATING_Measure_Temp(void)
 *
 * Returns the current temperature of the heating pad in degrees Celsius
 *
 * @return  (float)  [degrees Celsius]
 */
float HEATING_Measure_Temp(void);




/** calculateMovingAverage(float newAdcValue)
 *
 * Implements a moving average filter for ADC readings to reduce noise and provide a stable voltage measurement.
 *
 * @param  newAdcValue  The latest ADC reading to be added to the moving average filter.
 * @return  (float)  The filtered ADC value after applying the moving average.
 */
float calculateMovingAverage(float newAdcValue);



#endif  /* HEATING_H */

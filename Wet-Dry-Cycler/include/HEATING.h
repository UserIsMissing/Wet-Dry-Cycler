/**
 * @file    Heating.h
 *
 * Silicon Heating Pad control module 
 *
 * @author  Rafael Delwart
 * @date    20 Feb 2025
 *
 * @detail  This module controls the silicon heating pad using 
 * simple bang bang feedback. It reads the thermistor on the heating 
 * pad using the ADC, the raw ADC is convert to voltage then resistance
 * then to temperature using the exponential fitting made expirementally. 
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
#include <PWM.h>
#include <ADC.h>


// PINOUTS *******************************************************
#define THERMISTOR_PIN ADC_0 //IO SHIELD:36 STM: PA0, GPIOA, GPIO PIN 0 
#define HEATING_CONTROL_PIN PWM_4 //IO SHIELD:57 STM: PB6, GPIOB, GPIO PIN 6
//GND PIN IO SHIELD: 42


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


/** HEATING_Measure_Raw_ADC_Avg(void)
 *
 * Returns the moving average of the measured raw ADC.
 *
 * @return  (int)  [raw ADC]
 */
int HEATING_Measure_Raw_ADC_Avg(void);


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





/** HEATING_Measure_Temp_Avg(void)
 *  
 * Implements a moving average filter on the the measured temperature
 *  in order to get cleaner values
 * 
 * 
 */
float HEATING_Measure_Temp_Avg(void);


/** HEATING_Set_Temp(int Temp)
 *
 * If the measured temperature is below the desired temperature then the heating pad control pin
 * is set high, if the measured temperature is above the desired temperature then it turns the control pin low.
 * 
 *
 * @return  nothing
 */
void HEATING_Set_Temp(int Temp);




/** calculateMovingAverage(float newAdcValue)
 *
 * Implements a moving average filter for ADC readings to reduce noise and provide a stable voltage measurement.
 *
 * @param  newAdcValue  The latest ADC reading to be added to the moving average filter.
 * @return  (float)  The filtered ADC value after applying the moving average.
 */
float calculateMovingAverage(float newAdcValue);



#endif  /* HEATING_H */

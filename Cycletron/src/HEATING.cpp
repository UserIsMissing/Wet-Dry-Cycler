/**
 * @file    HEATING.cpp
 * @brief   Heating control module for thermistor-based pad using ESP32
 *
 * Measures temperature using a thermistor voltage divider and applies
 * bang-bang control to a heating pad via GPIO. Uses a moving average
 * filter for both ADC and temperature readings to smooth out noise.
 *
 * Author: Rafael Delwart
 * Date:   20 Feb 2025 (ESP32 update: May 2025)
 */

 #include <Arduino.h>
 #include "HEATING.h"
 #include <math.h>
 
 #define Serial0 Serial
 
 // === CONFIG ===
 #define HEATING_GPIO 14         // GPIO to control heater
 #define THERMISTOR_PIN 4        // ADC1_CHANNEL_0 = GPIO4 (ESP32-S3)
 #define MOVING_AVERAGE_WINDOW 80
 #define VREF 3.3                // ADC reference voltage
 #define ADC_RESOLUTION 4095.0   // 12-bit ADC = 4096 levels
 
 // === Thermistor constants ===
 const float R0   = 100000.0;     // Resistance at 25°C (reference temp)
 const float R1   = 4630.0;       // Fixed series resistor in voltage divider
 const float BETA = 3850.0;       // Beta value of thermistor
 const float T0   = 298.15;       // Reference temp in Kelvin (25°C)
 
 // === Buffers for Moving Average ===
 static int   adcBuffer[MOVING_AVERAGE_WINDOW] = {0}; // Circular buffer for ADC
 static int   adcIndex = 0, adcCount = 0;
 
 static float tempBuffer[MOVING_AVERAGE_WINDOW] = {0}; // Circular buffer for temperature
 static int   tempIndex = 0, tempCount = 0;
 
 // === API IMPLEMENTATION ===
 
 /**
  * @brief Initializes GPIO and ADC for heating system.
  *
  * Configures the heater control GPIO as OUTPUT and sets it LOW (off).
  * Sets ADC resolution to 12-bit. Should be called once at startup.
  */
 void HEATING_Init() {
   pinMode(HEATING_GPIO, OUTPUT);
   digitalWrite(HEATING_GPIO, LOW);  // Off by default
   analogReadResolution(12);         // 12-bit for ESP32
   Serial.println("[HEATING] Initialized GPIO and ADC");
 }
 
 /**
  * @brief Reads a raw ADC value from the thermistor pin.
  *
  * This is a direct analog read of the thermistor divider output.
  *
  * @return ADC value from 0 to 4095
  */
 int HEATING_Measure_Raw_ADC() {
   return analogRead(THERMISTOR_PIN);  // Returns 0–4095
 }
 
 /**
  * @brief Computes the moving average of recent ADC readings.
  *
  * Stores readings in a circular buffer and returns the average.
  * Helps reduce noise in thermistor signal.
  *
  * @return Averaged ADC reading
  */
 int HEATING_Measure_Raw_ADC_Avg() {
   int newAdc = HEATING_Measure_Raw_ADC();
   adcBuffer[adcIndex] = newAdc;
   adcIndex = (adcIndex + 1) % MOVING_AVERAGE_WINDOW;
   if (adcCount < MOVING_AVERAGE_WINDOW) adcCount++;
 
   int sum = 0;
   for (int i = 0; i < adcCount; i++) sum += adcBuffer[i];
   return sum / adcCount;
 }
 
 /**
  * @brief Converts averaged ADC value to voltage.
  *
  * Uses VREF and ADC resolution to compute the measured voltage.
  *
  * @return Voltage across thermistor (0.0–VREF)
  */
 float HEATING_Measure_Voltage() {
   int avg_adc = HEATING_Measure_Raw_ADC_Avg();
   return (VREF * avg_adc) / ADC_RESOLUTION;
 }
 
 /**
  * @brief Calculates thermistor resistance from voltage divider output.
  *
  * Uses voltage divider formula to solve for thermistor resistance.
  *
  * @return Thermistor resistance in Ohms
  */
 float HEATING_Measure_Resistance() {
   float Vout = HEATING_Measure_Voltage();
   if (Vout <= 0) return -1.0;
   return R1 * (VREF - Vout) / Vout;
 }
 
 /**
  * @brief Converts thermistor resistance to temperature using BETA formula.
  *
  * Applies the Steinhart-Hart simplified BETA equation to compute temperature.
  *
  * @return Temperature in degrees Celsius
  */
 float HEATING_Measure_Temp() {
   float R = HEATING_Measure_Resistance();
   if (R <= 0) return -273.15;  // Absolute zero on error
   float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(R / R0));
   return tempK - 273.15;
 }
 
 /**
  * @brief Computes moving average of temperature readings.
  *
  * Smooths out sensor noise by averaging recent temperature measurements.
  *
  * @return Averaged temperature in degrees Celsius
  */
 float HEATING_Measure_Temp_Avg() {
   float newTemp = HEATING_Measure_Temp();
   tempBuffer[tempIndex] = newTemp;
   tempIndex = (tempIndex + 1) % MOVING_AVERAGE_WINDOW;
   if (tempCount < MOVING_AVERAGE_WINDOW) tempCount++;
 
   float sum = 0;
   for (int i = 0; i < tempCount; i++) sum += tempBuffer[i];
   return sum / tempCount;
 }
 
 /**
  * @brief Simple bang-bang temperature controller.
  *
  * Turns heater ON if temperature is below setpoint.
  * Turns heater OFF if temperature is above or equal to setpoint.
  *
  * @param setpointCelsius Target temperature in Celsius
  */
 void HEATING_Set_Temp(int setpointCelsius) {
   float avgTemp = HEATING_Measure_Temp_Avg();
   if (avgTemp < setpointCelsius) {
     digitalWrite(HEATING_GPIO, HIGH);  // Turn ON
   } else {
     digitalWrite(HEATING_GPIO, LOW);   // Turn OFF
   }
 }
 
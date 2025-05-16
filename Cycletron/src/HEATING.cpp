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
 #define HEATING_GPIO 5         // GPIO to control heater
 #define THERMISTOR_PIN 4        // ADC1_CHANNEL_0 = GPIO4 (ESP32-S3)
 #define MOVING_AVERAGE_WINDOW 80 // Size of moving average window
 #define VREF 3.28                // ADC reference voltage
 #define ADC_RESOLUTION 4095.0   // 12-bit ADC = 4096 levels

 #define roomTempCalibrationOffset 18 // Calibration offset for room temperature

 #define TESTING_TEMP

 // === Thermistor constants ===
 const float R0   = 100000.0;     // Resistance at 25°C (reference temp)
 const float R1   = 4630.0;       // Fixed series resistor in voltage divider
 const float BETA = 3850.0;       // Beta value of thermistor
 const float T0   = 298.15;       // Reference temp in Kelvin (25°C)
 
 // === Buffers for Moving Average ===
 static float   mvBuffer[MOVING_AVERAGE_WINDOW] = {0}; // Circular buffer for MV readings
 static int   mvIndex = 0, mvCount = 0;
 
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
   analogSetAttenuation(ADC_11db);      // Set full voltage range 0–3.3V
   Serial.println("[HEATING] Initialized GPIO and ADC");
 }
 
 /**
  * @brief Reads a raw MilliVolt value from the thermistor pin.
  *
  * This is a direct analog read of the thermistor divider output.
  *
  * @return ADC value from 0 to 4095
  */
 int HEATING_Measure_Raw_MV() {
  return analogReadMilliVolts(THERMISTOR_PIN) + roomTempCalibrationOffset;
}
 
 /**
  * @brief Computes the moving average of recent MilliVolt readings.
  *
  * Stores readings in a circular buffer and returns the average.
  * Helps reduce noise in thermistor signal.
  *
  * @return Averaged ADC reading
  */
 float HEATING_Measure_AVG_MV() {
  float newMV = HEATING_Measure_Raw_MV() / 1000.0; // Convert to Volts
  mvBuffer[mvIndex] = newMV;
  mvIndex = (mvIndex + 1) % MOVING_AVERAGE_WINDOW;
  if (mvCount < MOVING_AVERAGE_WINDOW) mvCount++;

  float sum = 0;
  for (int i = 0; i < mvCount; i++) {
      sum += mvBuffer[i];
  }

  float avg = sum / mvCount;
  return avg;
}
 

 /**
  * @brief Calculates thermistor resistance from voltage divider output.
  *
  * Uses voltage divider formula to solve for thermistor resistance.
  *
  * @return Thermistor resistance in Ohms
  */
 float HEATING_Measure_Resistance() {
   float Vout = HEATING_Measure_AVG_MV();
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
 



#ifdef TESTING_TEMP

#include <Arduino.h>
#include "HEATING.h"

void setup() {
  Serial.begin(115200);
  delay(2000); // Allow USB Serial to connect

  HEATING_Init();
}

void loop() {
  Serial.println(">Raw Voltage: " + String(HEATING_Measure_Raw_MV()));
  Serial.println(">Voltage AVG: " + String(HEATING_Measure_AVG_MV(), 5) + " V" );
  Serial.println(">Resistance: " + String(HEATING_Measure_Resistance() / 1000.0, 3) + " kOhm");
  Serial.println(">Temperature: " + String(HEATING_Measure_Temp(), 3) + " °C");
  Serial.println(">Temperature AVG: " + String(HEATING_Measure_Temp_Avg(), 3) + " °C");

  HEATING_Set_Temp(90); // Bang-bang control target temperature

  delay(10); // Delay to make output readable
}

#endif // TESTING_TEMP
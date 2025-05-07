#include <Arduino.h>
#include "HEATING.h"
#include <math.h>

#define Serial0 Serial


// === CONFIG ===
#define HEATING_GPIO 14         // GPIO to control heater
#define THERMISTOR_PIN 4        // ADC1_CHANNEL_0 = GPIO4 (ESP32-S3)
#define MOVING_AVERAGE_WINDOW 80
#define VREF 3.3
#define ADC_RESOLUTION 4095.0   // Arduino default is 12-bit = 4096 levels

// === Thermistor constants ===
const float R0   = 100000.0;
const float R1   = 4630.0;
const float BETA = 3850.0;
const float T0   = 298.15;  // in Kelvin

// === Buffers ===
static int   adcBuffer[MOVING_AVERAGE_WINDOW] = {0};
static int   adcIndex = 0, adcCount = 0;

static float tempBuffer[MOVING_AVERAGE_WINDOW] = {0};
static int   tempIndex = 0, tempCount = 0;

// === API IMPLEMENTATION ===

void HEATING_Init() {
  pinMode(HEATING_GPIO, OUTPUT);
  digitalWrite(HEATING_GPIO, LOW);  // Off by default
  analogReadResolution(12);         // 12-bit for ESP32
  Serial.println("[HEATING] Initialized GPIO and ADC");
}

int HEATING_Measure_Raw_ADC() {
  return analogRead(THERMISTOR_PIN);  // Returns 0–4095
}

int HEATING_Measure_Raw_ADC_Avg() {
  int newAdc = HEATING_Measure_Raw_ADC();
  adcBuffer[adcIndex] = newAdc;
  adcIndex = (adcIndex + 1) % MOVING_AVERAGE_WINDOW;
  if (adcCount < MOVING_AVERAGE_WINDOW) adcCount++;

  int sum = 0;
  for (int i = 0; i < adcCount; i++) sum += adcBuffer[i];
  return sum / adcCount;
}

float HEATING_Measure_Voltage() {
  int avg_adc = HEATING_Measure_Raw_ADC_Avg();
  return (VREF * avg_adc) / ADC_RESOLUTION;
}

float HEATING_Measure_Resistance() {
  float Vout = HEATING_Measure_Voltage();
  if (Vout <= 0) return -1.0;
  return R1 * (VREF - Vout) / Vout;
}

float HEATING_Measure_Temp() {
  float R = HEATING_Measure_Resistance();
  if (R <= 0) return -273.15;
  float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(R / R0));
  return tempK - 273.15;
}

float HEATING_Measure_Temp_Avg() {
  float newTemp = HEATING_Measure_Temp();
  tempBuffer[tempIndex] = newTemp;
  tempIndex = (tempIndex + 1) % MOVING_AVERAGE_WINDOW;
  if (tempCount < MOVING_AVERAGE_WINDOW) tempCount++;

  float sum = 0;
  for (int i = 0; i < tempCount; i++) sum += tempBuffer[i];
  return sum / tempCount;
}

void HEATING_Set_Temp(int setpointCelsius) {
  float avgTemp = HEATING_Measure_Temp_Avg();
  if (avgTemp < setpointCelsius) {
    digitalWrite(HEATING_GPIO, HIGH);  // Turn ON
  } else {
    digitalWrite(HEATING_GPIO, LOW);   // Turn OFF
  }
}

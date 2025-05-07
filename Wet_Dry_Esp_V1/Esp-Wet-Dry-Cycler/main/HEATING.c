/**
 * @file    Heating.c
 *
 * @brief   Silicon Heating Pad control module for ESP32 (self-contained)
 *
 * @author  Rafael Delwart
 * @date    20 Feb 2025 (ESP32 update: May 2025)
 */

 #include "Heating.h"
 #include "esp_log.h"
 #include "driver/gpio.h"
 #include "esp_adc/adc_oneshot.h"
 #include <math.h>
 
 // === DEFINES ===================================================
 
 #define MOVING_AVERAGE_WINDOW 80
 #define VREF 3.3                  // Reference voltage for ADC
 #define ADC_RESOLUTION 4096      // 12-bit resolution
 #define HEATING_CONTROL_GPIO GPIO_NUM_14   // GPIO to control heater
 #define HEATING_ADC_CHANNEL ADC_CHANNEL_0  // ADC1 Channel 0 = GPIO36
 
 // === CONSTANTS & STATE =========================================
 
 const float R0 = 100000.0;    // Resistance at 25°C
 float R1 = 4630;              // Series resistor in voltage divider
 const float BETA = 3850.0;    // Thermistor beta value
 const float T0 = 298.15;      // 25°C in Kelvin
 
 // ADC handle and initialization flag
 static adc_oneshot_unit_handle_t adc_handle;
 static bool adc_initialized = false;
 
 // Buffers for smoothing
 static int adcBuffer[MOVING_AVERAGE_WINDOW] = {0};
 static int adcIndex = 0, adcCount = 0;
 
 static float tempBuffer[MOVING_AVERAGE_WINDOW] = {0};
 static int tempIndex = 0, tempCount = 0;
 
 // === FUNCTION IMPLEMENTATIONS ==================================
 
 /**
  * @brief Initializes GPIO for heating control and sets up ADC
  */
 void HEATING_Init(void)
 {
     // Configure heating pad control GPIO
     gpio_config_t io_conf = {
         .pin_bit_mask = (1ULL << HEATING_CONTROL_GPIO),
         .mode = GPIO_MODE_OUTPUT,
         .pull_down_en = GPIO_PULLDOWN_DISABLE,
         .pull_up_en = GPIO_PULLUP_DISABLE,
         .intr_type = GPIO_INTR_DISABLE
     };
     gpio_config(&io_conf);
     gpio_set_level(HEATING_CONTROL_GPIO, 0); // Initially off
 
     // Initialize ADC if not already done
     if (!adc_initialized)
     {
         adc_oneshot_unit_init_cfg_t init_config = {
             .unit_id = ADC_UNIT_1
         };
         ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
 
         adc_oneshot_chan_cfg_t chan_cfg = {
             .bitwidth = ADC_BITWIDTH_DEFAULT,
             .atten = ADC_ATTEN_DB_11,
         };
         ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, HEATING_ADC_CHANNEL, &chan_cfg));
 
         adc_initialized = true;
     }
 
     ESP_LOGI("HEATING", "Heating module initialized (GPIO + ADC)");
 }
 
 /**
  * @brief Reads raw ADC value from the thermistor channel.
  * @return Raw ADC value or -1 on error
  */
 int HEATING_Measure_Raw_ADC(void)
 {
     int adc_raw = 0;
     esp_err_t err = adc_oneshot_read(adc_handle, HEATING_ADC_CHANNEL, &adc_raw);
     if (err != ESP_OK)
     {
         ESP_LOGE("HEATING", "ADC read failed: %s", esp_err_to_name(err));
         return -1;
     }
     return adc_raw;
 }
 
 /**
  * @brief Calculates moving average of raw ADC readings
  * @return Averaged ADC value
  */
 int HEATING_Measure_Raw_ADC_Avg(void)
 {
     int newAdc = HEATING_Measure_Raw_ADC();
     if (newAdc < 0) return -1;
 
     adcBuffer[adcIndex] = newAdc;
     adcIndex = (adcIndex + 1) % MOVING_AVERAGE_WINDOW;
     if (adcCount < MOVING_AVERAGE_WINDOW) adcCount++;
 
     int sum = 0;
     for (int i = 0; i < adcCount; i++) sum += adcBuffer[i];
     return sum / adcCount;
 }
 
 /**
  * @brief Converts averaged ADC value to thermistor voltage
  * @return Voltage in volts
  */
 float HEATING_Measure_Voltage(void)
 {
     int avg_adc = HEATING_Measure_Raw_ADC_Avg();
     if (avg_adc < 0) return -1.0;
     return (VREF * ((float)avg_adc / ADC_RESOLUTION));
 }
 
 /**
  * @brief Calculates thermistor resistance based on voltage divider formula
  * @return Resistance in Ohms
  */
 float HEATING_Measure_Resistance(void)
 {
     float Vout = HEATING_Measure_Voltage();
     if (Vout <= 0) return -1.0;
     return R1 * (VREF - Vout) / Vout;
 }
 
 /**
  * @brief Converts thermistor resistance to temperature using BETA formula
  * @return Temperature in °C
  */
 float HEATING_Measure_Temp(void)
 {
     float resistance = HEATING_Measure_Resistance();
     if (resistance <= 0) return -273.15;
     float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(resistance / R0));
     return tempK - 273.15;
 }
 
 /**
  * @brief Applies moving average filter to temperature readings
  * @return Averaged temperature in °C
  */
 float HEATING_Measure_Temp_Avg(void)
 {
     float newTemp = HEATING_Measure_Temp();
     tempBuffer[tempIndex] = newTemp;
     tempIndex = (tempIndex + 1) % MOVING_AVERAGE_WINDOW;
     if (tempCount < MOVING_AVERAGE_WINDOW) tempCount++;
 
     float sum = 0;
     for (int i = 0; i < tempCount; i++) sum += tempBuffer[i];
     return sum / tempCount;
 }
 
 /**
  * @brief Turns the heating element on/off using bang-bang control
  * @param setpointCelsius Desired temperature setpoint
  */
 void HEATING_Set_Temp(int setpointCelsius)
 {
     float avgTemp = HEATING_Measure_Temp_Avg();
     if (avgTemp < (float)setpointCelsius)
     {
         gpio_set_level(HEATING_CONTROL_GPIO, 1); // Turn ON
     }
     else
     {
         gpio_set_level(HEATING_CONTROL_GPIO, 0); // Turn OFF
     }
 }
 
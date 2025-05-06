#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "driver/gpio.h"
#include "cJSON.h"
#include "esp_adc/adc_oneshot.h"

#include "HEATING.h" // Heating module header

// Wi-Fi credentials
#define WIFI_SSID "DonnaHouse"
#define WIFI_PASS "guessthepassword"

// Hardware setup
#define LED_GPIO 2 // GPIO controlling LED

#define MIXING_MOTOR_1_GPIO 11    // GPIO for Mixing Motor 1
#define MIXING_MOTOR_2_GPIO 12    // GPIO for Mixing Motor 2
#define MIXING_MOTOR_3_GPIO 13    // GPIO for Mixing Motor 3
#define ADC_CHANNEL ADC_CHANNEL_0 // GPIO4 by default (ADC1_CH0)

// Server endpoints
#define SERVER_URL_LED_STATE "http://10.0.0.166:5000/led-state"
#define SERVER_URL_ADC_DATA  "http://10.0.0.166:5000/adc-data" // Still named "adc-data", though now used for temp

// Log tags for debugging
static const char *TAG_WIFI = "WIFI";
static const char *TAG_HTTP = "HTTP";
static const char *TAG_ADC = "ADC";

// Global flag to track WiFi connection status
static bool wifi_connected = false;

/**
 * @brief Callback function for WiFi and IP events.
 * Handles connection, reconnection, and IP acquisition.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG_WIFI, "Disconnected. Reconnecting...");
        wifi_connected = false;
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG_WIFI, "WiFi connected and IP acquired.");
        wifi_connected = true;
    }
}

/**
 * @brief Initialize Wi-Fi in station (client) mode.
 */
void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}



/**
 * @brief Sends a JSON-formatted temperature reading to the server.
 * Called periodically from the polling task.
 *
 * @param temp_val Temperature in Celsius (float)
 */
void send_temperature_reading(float temp_val)
{
    char post_data[64];
    snprintf(post_data, sizeof(post_data), "{\"temperature\": %.2f}", temp_val);

    esp_http_client_config_t config = {
        .url = SERVER_URL_ADC_DATA,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG_HTTP, "Failed to initialize HTTP client for temperature");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG_ADC, "Sent Temperature: %.2f °C", temp_val);
    }
    else
    {
        ESP_LOGE(TAG_ADC, "Failed to send temperature: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/**
 * @brief Controls a given GPIO pin based on the string "on"/"off"
 */
void handle_gpio_control(gpio_num_t gpio_num, const char *state)
{
    int level = (strcmp(state, "on") == 0) ? 0 : 1;
    gpio_set_level(gpio_num, level);
    ESP_LOGI(TAG_HTTP, "GPIO %d set to %s", gpio_num, state);
}

/**
 * @brief Polls the server periodically to update GPIO states
 * and send the current heating pad temperature.
 */
void poll_server_task(void *pvParameters)
{
    // Wait for Wi-Fi connection
    while (!wifi_connected)
    {
        ESP_LOGI(TAG_HTTP, "Waiting for WiFi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Set up HTTP client
    esp_http_client_config_t config = {
        .url = SERVER_URL_LED_STATE,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG_HTTP, "Failed to init HTTP client");
        vTaskDelete(NULL);
    }

    esp_http_client_set_header(client, "Accept", "application/json");

    // Store previous states to avoid redundant writes
    static char last_led_state[8] = "";
    static char last_mix1_state[8] = "";
    static char last_mix2_state[8] = "";
    static char last_mix3_state[8] = "";

    while (1)
    {
        // === GET ===
        esp_http_client_set_method(client, HTTP_METHOD_GET);
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK)
        {
            esp_http_client_fetch_headers(client);

            char buffer[256] = {0};
            int total_read = 0;
            while (1)
            {
                int len = esp_http_client_read(client, buffer + total_read, sizeof(buffer) - 1 - total_read);
                if (len <= 0)
                    break;
                total_read += len;
            }
            buffer[total_read] = '\0';

            cJSON *root = cJSON_Parse(buffer);
            if (root)
            {
                cJSON *led = cJSON_GetObjectItem(root, "led");
                if (cJSON_IsString(led) && strcmp(led->valuestring, last_led_state) != 0)
                {
                    strcpy(last_led_state, led->valuestring);
                    handle_gpio_control(GPIO_NUM_2, led->valuestring);
                }

                cJSON *mix1 = cJSON_GetObjectItem(root, "mix1");
                if (cJSON_IsString(mix1) && strcmp(mix1->valuestring, last_mix1_state) != 0)
                {
                    strcpy(last_mix1_state, mix1->valuestring);
                    handle_gpio_control(MIXING_MOTOR_1_GPIO, mix1->valuestring);
                }

                cJSON *mix2 = cJSON_GetObjectItem(root, "mix2");
                if (cJSON_IsString(mix2) && strcmp(mix2->valuestring, last_mix2_state) != 0)
                {
                    strcpy(last_mix2_state, mix2->valuestring);
                    handle_gpio_control(MIXING_MOTOR_2_GPIO, mix2->valuestring);
                }

                cJSON *mix3 = cJSON_GetObjectItem(root, "mix3");
                if (cJSON_IsString(mix3) && strcmp(mix3->valuestring, last_mix3_state) != 0)
                {
                    strcpy(last_mix3_state, mix3->valuestring);
                    handle_gpio_control(MIXING_MOTOR_3_GPIO, mix3->valuestring);
                }

                cJSON_Delete(root);
            }
            else
            {
                ESP_LOGW(TAG_HTTP, "Failed to parse JSON");
            }

            esp_http_client_close(client);
        }
        else
        {
            ESP_LOGE(TAG_HTTP, "HTTP GET failed: %s", esp_err_to_name(err));
        }

        // === POST temperature reading ===
        float temp = HEATING_Measure_Temp_Avg();
        send_temperature_reading(temp);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

/**
 * @brief Main entry point: initializes WiFi, GPIO, and launches polling task.
 */
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());                                // Flash for WiFi
    ESP_ERROR_CHECK(gpio_reset_pin(LED_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT));

    HEATING_Init();  // Initialize heating GPIO
    wifi_init_sta(); // Connect to WiFi

    xTaskCreate(poll_server_task, "poll_server", 8192, NULL, 5, NULL); // Start control task
}


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

// Wi-Fi credentials
#define WIFI_SSID "DonnaHouse"
#define WIFI_PASS "guessthepassword"

// Hardware setup
#define LED_GPIO 2                // GPIO controlling onboard LED
#define ADC_CHANNEL ADC_CHANNEL_0 // GPIO4 by default (ADC1_CH0)

// Server endpoints
#define SERVER_URL_LED_STATE "http://10.0.0.166:5000/led-state" // change (10.0.0.166) to your server IP
#define SERVER_URL_ADC_DATA "http://10.0.0.166:5000/adc-data"   // change (10.0.0.166) to your server IP

// Log tags for easy debugging
static const char *TAG_WIFI = "WIFI"; // Logs related to WiFi connection
static const char *TAG_HTTP = "HTTP"; // Logs related to HTTP requests
static const char *TAG_ADC = "ADC";   // Logs related to ADC readings

// Global flag to track WiFi connection status
static bool wifi_connected = false;

/**
 * @brief Callback function for WiFi and IP events.
 * Handles connection, reconnection, and IP acquisition.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect(); // Start connecting to the AP
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG_WIFI, "Disconnected. Reconnecting...");
        wifi_connected = false; // s
        esp_wifi_connect();     // Attempt reconnection
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG_WIFI, "WiFi connected and IP acquired.");
        wifi_connected = true; // Set global flag
    }
}

/**
 * @brief Initialize Wi-Fi in station (client) mode.
 */
void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());                // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Create default event loop
    esp_netif_create_default_wifi_sta();              // Create default Wi-Fi station

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Initialize Wi-Fi driver

    // Register Wi-Fi and IP event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Configure Wi-Fi credentials
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));               // Set mode to station
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // Apply config
    ESP_ERROR_CHECK(esp_wifi_start());                               // Start Wi-Fi
}

/**
 * @brief Read raw value from ADC (ADC1, Channel 0 by default).
 * Initializes ADC only once (static flag).
 *
 * @return int ADC raw value or -1 on error.
 */
int read_adc_value(void)
{
    static bool initialized = false;
    static adc_oneshot_unit_handle_t adc_handle;

    if (!initialized)
    {
        // Initialize ADC unit
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

        // Configure ADC channel
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_11,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

        initialized = true;
    }

    int adc_raw = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_ADC, "ADC read failed: %s", esp_err_to_name(err));
        return -1;
    }
    return adc_raw;
}

/**
 * @brief Sends a JSON-formatted ADC value to the remote server via HTTP POST.
 *
 * @param adc_val Integer ADC value to send.
 */
void send_adc_reading(int adc_val)
{
    char post_data[64];
    snprintf(post_data, sizeof(post_data), "{\"adc\": %d}", adc_val);

    esp_http_client_config_t config = {
        .url = SERVER_URL_ADC_DATA,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG_HTTP, "Failed to initialize HTTP client for ADC");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG_ADC, "Sent ADC: %d", adc_val);
    }
    else
    {
        ESP_LOGE(TAG_ADC, "Failed to send ADC: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/**
 * @brief Task to continuously poll a remote server for LED state,
 *        and send ADC readings.
 *
 * This function:
 * 1. Waits for Wi-Fi connection.
 * 2. Sends HTTP GET to get LED state ("on"/"off").
 * 3. Parses JSON and sets the GPIO level accordingly.
 * 4. Sends the current ADC value via HTTP POST.
 */
void poll_server_task(void *pvParameters)
{
    // Wait for Wi-Fi connection
    while (!wifi_connected)
    {
        ESP_LOGI(TAG_HTTP, "Waiting for WiFi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // HTTP client configuration
    esp_http_client_config_t config = {
        .url = SERVER_URL_LED_STATE,
        .timeout_ms = 3000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG_HTTP, "Failed to init HTTP client");
        vTaskDelete(NULL);
    }

    esp_http_client_set_header(client, "Accept", "application/json");

    while (1)
    {
        esp_http_client_set_method(client, HTTP_METHOD_GET);

        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK)
        {
            esp_http_client_fetch_headers(client);
            char buffer[128] = {0};
            int total_read = 0;

            // Read full response body
            while (1)
            {
                int len = esp_http_client_read(client, buffer + total_read, sizeof(buffer) - 1 - total_read);
                if (len <= 0)
                    break;
                total_read += len;
            }
            buffer[total_read] = ' ';

            // Parse JSON response to control LED
            cJSON *root = cJSON_Parse(buffer);
            if (root)
            {
                cJSON *led = cJSON_GetObjectItem(root, "led");
                if (cJSON_IsString(led))
                {
                    if (strcmp(led->valuestring, "on") == 0)
                    {
                        gpio_set_level(LED_GPIO, 0); // Active-low: 0 = ON
                        ESP_LOGI(TAG_HTTP, "LED ON");
                    }
                    else if (strcmp(led->valuestring, "off") == 0)
                    {
                        gpio_set_level(LED_GPIO, 1); // 1 = OFF
                        ESP_LOGI(TAG_HTTP, "LED OFF");
                    }
                }
                cJSON_Delete(root);
            }
            else
            {
                ESP_LOGW(TAG_HTTP, "Invalid JSON response");
            }

            esp_http_client_close(client);
        }
        else
        {
            ESP_LOGE(TAG_HTTP, "HTTP GET failed: %s", esp_err_to_name(err));
        }

        // Read ADC and send to server
        int adc_val = read_adc_value();
        if (adc_val >= 0)
        {
            send_adc_reading(adc_val);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Wait before next poll
    }
}

/**
 * @brief Main application entry point.
 * Initializes NVS, GPIO, Wi-Fi, and launches polling task.
 */
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init()); // Initialize flash storage for WiFi credentials

    ESP_ERROR_CHECK(gpio_reset_pin(LED_GPIO));                       // Reset LED GPIO
    ESP_ERROR_CHECK(gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT)); // Set LED pin as output

    wifi_init_sta();                                                   // Start Wi-Fi connection
    xTaskCreate(poll_server_task, "poll_server", 8192, NULL, 5, NULL); // Start background task
}

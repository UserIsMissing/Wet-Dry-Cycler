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

#define WIFI_SSID "DonnaHouse"
#define WIFI_PASS "guessthepassword"
#define LED_GPIO 2
#define SERVER_URL "http://10.0.0.166:5000/led-state"

static const char *TAG = "HTTP_LED";
static bool wifi_connected = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connected to WiFi!");
        wifi_connected = true;
    }
}

void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void poll_server(void *pvParameters) {
    while (!wifi_connected) {
        ESP_LOGI(TAG, "Waiting for WiFi...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .timeout_ms = 3000,
        .is_async = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Accept", "application/json");

    while (1) {
        esp_http_client_set_method(client, HTTP_METHOD_GET);
        esp_http_client_set_url(client, SERVER_URL);

        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK) {
            esp_http_client_fetch_headers(client);

            char buffer[128] = {0};
            int total_read = 0;

            while (1) {
                int len = esp_http_client_read(client, buffer + total_read, sizeof(buffer) - 1 - total_read);
                if (len <= 0) break;
                total_read += len;
            }

            if (total_read > 0) {
                buffer[total_read] = '\0';
                ESP_LOGI(TAG, "Server response: '%s'", buffer);

                // Trim whitespace
                char *start = buffer;
                while (*start == ' ' || *start == '\n' || *start == '\r') start++;
                char *end = start + strlen(start) - 1;
                while (end > start && (*end == ' ' || *end == '\n' || *end == '\r')) *end-- = '\0';

                cJSON *root = cJSON_Parse(start);
                if (root != NULL) {
                    cJSON *led_val = cJSON_GetObjectItem(root, "led");
                    if (cJSON_IsString(led_val) && led_val->valuestring != NULL) {
                        ESP_LOGI(TAG, "LED Command: %s", led_val->valuestring);
                        if (strcmp(led_val->valuestring, "on") == 0) {
                            gpio_set_level(LED_GPIO, 1);                        
                            ESP_LOGI(TAG, "Turned On LED");
                        } else if (strcmp(led_val->valuestring, "off") == 0) {
                            gpio_set_level(LED_GPIO, 0vbfcgf);
                            ESP_LOGI(TAG, "Turned Off LED");

                        } else {
                            ESP_LOGW(TAG, "Unknown LED value: %s", led_val->valuestring);
                        }
                    } else {
                        ESP_LOGW(TAG, "Missing or invalid 'led' key in JSON");
                    }
                    cJSON_Delete(root);
                } else {
                    ESP_LOGW(TAG, "Failed to parse JSON response");
                }
            } else {
                ESP_LOGW(TAG, "No data read from server");
            }

            esp_http_client_close(client);
        } else {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    esp_err_t ret;

    ret = gpio_reset_pin(LED_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset GPIO %d: %s", LED_GPIO, esp_err_to_name(ret));
    }

    ret = gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d as output: %s", LED_GPIO, esp_err_to_name(ret));
    }

    nvs_flash_init();
    wifi_init_sta();
    xTaskCreate(&poll_server, "poll_server", 8192, NULL, 5, NULL);
}
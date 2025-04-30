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
#define LED_GPIO 2  // Replace with your actual LED GPIO pin
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
        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            int content_length = esp_http_client_get_content_length(client);
            ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d", status_code, content_length);

            if (status_code == 200) {
                char buffer[128] = {0};
                int read_len = esp_http_client_read_response(client, buffer, sizeof(buffer) - 1);

                if (read_len >= 0) {
                    buffer[read_len] = '\0';
                    ESP_LOGI(TAG, "Server response: '%s'", buffer);
                    ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, read_len, ESP_LOG_INFO);

                    // Trim whitespace (optional but helpful for robustness)
                    char *start = buffer;
                    while (*start == ' ' || *start == '\n' || *start == '\r') start++;
                    char *end = start + strlen(start) - 1;
                    while (end > start && (*end == ' ' || *end == '\n' || *end == '\r')) *end-- = '\0';

                    cJSON *root = cJSON_Parse(start);
                    if (root != NULL) {
                        cJSON *led_val = cJSON_GetObjectItem(root, "led");
                        if (cJSON_IsString(led_val) && led_val->valuestring != NULL) {
                            ESP_LOGI(TAG, "Setting LED: %s", led_val->valuestring);
                            if (strcmp(led_val->valuestring, "on") == 0) {
                                gpio_set_level(LED_GPIO, 1);
                            } else if (strcmp(led_val->valuestring, "off") == 0) {
                                gpio_set_level(LED_GPIO, 0);
                            } else {
                                ESP_LOGW(TAG, "Unexpected LED value: %s", led_val->valuestring);
                            }
                        }
                        cJSON_Delete(root);
                    } else {
                        ESP_LOGW(TAG, "Failed to parse JSON response");
                    }
                } else {
                    ESP_LOGW(TAG, "Failed to read response");
                }
            } else {
                ESP_LOGW(TAG, "Non-200 response");
            }
        } else {
            ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    nvs_flash_init();
    wifi_init_sta();
    xTaskCreate(&poll_server, "poll_server", 8192, NULL, 5, NULL);
}

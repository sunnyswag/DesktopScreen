/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#include "http_request.h"
#include "http_request_rules.h"

static const char *TAG = "HTTP_CLIENT";

QueueHandle_t http_request_event_queue;

typedef struct
{
    char *data;
    size_t length;
    esp_err_t error;
    void (*parse)(char *text);
} http_response_t;

static void clear_http_response(http_response_t *response)
{
    free(response->data);
    response->data = NULL;
    response->length = 0;
}

static esp_err_t append_http_response(http_response_t *response, const char *data, int data_len)
{
    if (response->error != ESP_OK || data_len <= 0)
    {
        return response->error;
    }
    size_t length = response->length + (size_t)data_len;
    char *buffer = realloc(response->data, length + 1);
    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for HTTP response");
        response->error = ESP_ERR_NO_MEM;
        return response->error;
    }
    memcpy(buffer + response->length, data, data_len);
    buffer[length] = '\0';
    response->data = buffer;
    response->length = length;
    return ESP_OK;
}

static void log_http_disconnect_error(void *data)
{
    if (data == NULL)
    {
        return;
    }
    int mbedtls_err = 0;
    esp_err_t err = esp_tls_get_and_clear_last_error(data, &mbedtls_err, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
        ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *response = evt->user_data;
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        return append_http_response(response, evt->data, evt->data_len);
    case HTTP_EVENT_ON_FINISH:
        // Parse only after the complete, NUL-terminated response has arrived.
        if (!esp_http_client_is_complete_data_received(evt->client))
        {
            response->error = ESP_FAIL;
        }
        if (response->error == ESP_OK && response->data != NULL)
        {
            printf("%s\n", response->data);
            response->parse(response->data);
        }
        clear_http_response(response);
        break;
    case HTTP_EVENT_DISCONNECTED:
        log_http_disconnect_error(evt->data);
        clear_http_response(response);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void perform_http_get(const char *url, void (*parse)(char *text))
{
    http_response_t response = {.parse = parse};
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_GET,
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &response,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK && response.error == ESP_OK)
    {
        ESP_LOGI(TAG, "Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err != ESP_OK ? err : response.error));
    }
    esp_http_client_cleanup(client);
    clear_http_response(&response);
}

void ds_http_request_type(HTTP_REQUEST_TYPE_E type)
{
    HTTP_REQUEST_TYPE_E evt;
    evt = type;
    xQueueSend(http_request_event_queue, &evt, 0);
}

void ds_http_request_all() { ds_http_request_type(HTTP_GET_WEATHER); }

static void http_request_task(void *pvParameters)
{
    while (1)
    {
        HTTP_REQUEST_TYPE_E evt;
        xQueueReceive(http_request_event_queue, &evt, portMAX_DELAY);
        ESP_LOGI(TAG, "http_get_task %d", evt);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        const http_request_rule_t *rule = ds_http_request_rule_get(evt);
        if (rule == NULL)
        {
            continue;
        }
        char url[160];
        int length = rule->gen_url(url, sizeof(url));
        if (length < 0 || (size_t)length >= sizeof(url))
        {
            ESP_LOGE(TAG, "Failed to generate HTTP request URL for type %d", evt);
            continue;
        }
        perform_http_get(url, rule->parse_func);
    }
}

void ds_http_request_init(void)
{
    http_request_event_queue = xQueueCreate(3, sizeof(HTTP_REQUEST_TYPE_E));
    xTaskCreate(&http_request_task, "http_request_task", 4096, NULL, 5, NULL);
}

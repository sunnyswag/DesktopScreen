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
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "http_request.h"
#include "ui_weatherpage.h"
#include "nvs.h"
#include "system_data.h"

static const char *TAG = "HTTP_CLIENT";

QueueHandle_t http_request_event_queue;

static cJSON *parse_json_response(const char *text)
{
    // Some endpoints wrap JSON in a JavaScript assignment.
    const char *json = strchr(text, '{');
    return json ? cJSON_Parse(json) : NULL;
}
static void cjson_time_info(char *text)
{
    cJSON *root= parse_json_response(text);
    cJSON *time = cJSON_GetObjectItem(root, "sysTime1");
    if (cJSON_IsString(time) && strlen(time->valuestring) >= 14)
    {
        const char *value = time->valuestring;
        ESP_LOGI(TAG,"sysTime:%s", value);
    uint8_t
    hour = (value[8] - '0')*10+ value[9] - '0';
        uint8_t
    minute = (value[10] - '0')*10+ value[11] - '0';
        uint8_t
    second = (value[12] - '0')*10+ value[13] - '0';
    update_system_time(hour,minute,second);
}
    cJSON_Delete(root);
}

static void update_weather_value(const cJSON *object, const char *key, VALUE_TYPE_T type,
                                 const char *label)
{
    cJSON *value = cJSON_GetObjectItem(object, key);
    if (cJSON_IsString(value))
    {
        ds_ui_weather_set(type, value->valuestring);
        ESP_LOGI(TAG, "%s:%s", label, value->valuestring);
    }
}
void cjson_weather_info(char *text)
{
    cJSON *root= parse_json_response(text);
    cJSON *results = cJSON_GetObjectItem(root, "results");
    cJSON *item = cJSON_GetArrayItem(results,0);

        cJSON *location = cJSON_GetObjectItem(item, "location");
        cJSON *now = cJSON_GetObjectItem(item, "now");
        if(cJSON_IsObject(location)&& cJSON_IsObject(now))
        {
        update_weather_value(location,"name", VALUE_CITY, "city");
        update_weather_value(now,"text", VALUE_WEATHER, "weather");
        update_weather_value(now,"code", VALUE_CODE, "code");
        update_weather_value(now,"temperature", VALUE_TEMP, "temperatur");
    }
    cJSON_Delete(root);
}

static void log_json_string(const cJSON *object, const char *key, const char *label)
{
    cJSON *value = cJSON_GetObjectItem(object, key);
    if (cJSON_IsString(value))
    {
        ESP_LOGI(TAG, "%s:%s", label, value->valuestring);
    }
}
static void cjson_city_info(char *text)
{
    cJSON *root= parse_json_response(text);
    log_json_string(root, "cname", "name");
    log_json_string(root, "cid", "cid");
    cJSON_Delete(root);
}

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
    size_t length = response->length + (size_t)data_len; char*buffer = realloc(response->data, length + 1);
            if (buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for HTTP response");
        response->error = ESP_ERR_NO_MEM;
                        return response->error;
                }
                memcpy(buffer + response->length,data,data_len);
    buffer[length] = '\0';
    response->data = buffer;
    response->length = length;
    return ESP_OK;
            }

static void log_http_disconnect_error(void *data)
{
            if (data == NULL) {
        return;
            }
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(data, &mbedtls_err, NULL);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t*response = evt->user_data;
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
        return append_http_response(response, evt->data, evt->data_len);
    case HTTP_EVENT_ON_FINISH:
        // Parse only after the complete, NUL-terminated response has arrived.
                    if (!esp_http_client_is_complete_data_received(evt->client))
        {
            response->error = ESP_FAIL;
                        }
        if(response->error == ESP_OK && response->data != NULL) {
            printf("%s\n", response->data);
            response->parse(response->data);
            }
        clear_http_response(response);
            break;
        case HTTP_EVENT_DISCONNECTED:
        log_http_disconnect_error(evt->data);
        clear_http_response (response);
        break;
    default:
            break;
    }
    return ESP_OK;
}

static void perform_http_get(const char *url, void (*parse)(char *text)) {
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

	if(err == ESP_OK && response.error == ESP_OK)
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

void http_weather_get(void){
    char url[160];
    char *city = ds_nvs_read_city();
    int length = snprintf(url, sizeof(url),
                          "http://api.seniverse.com/v3/weather/now.json"
                          "?key=SmazqPcltzTft-X3v&location=%s&language=zh-Hans&unit=c",
                          city ? city : "guangzhou");
        free(city);

	if(length < 0 || (size_t)length >= sizeof(url))
	{
        ESP_LOGE(TAG, "Weather request URL is too long");
        return;
	}
    perform_http_get(url, cjson_weather_info);
}


void ds_http_request_type(HTTP_REQUEST_TYPE_E type){
    HTTP_REQUEST_TYPE_E evt;
	evt = type;
	xQueueSend(http_request_event_queue, &evt, 0);
}

void ds_http_request_all(){
    ds_http_request_type(HTTP_GET_WEATHER);
}

static void http_request_task(void *pvParameters)
{
    while(1) {
        HTTP_REQUEST_TYPE_E evt;
        xQueueReceive(http_request_event_queue, &evt, portMAX_DELAY);
        ESP_LOGI(TAG, "http_get_task %d",evt);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        switch (evt)
        {
        case HTTP_GET_TIME:
            perform_http_get("http://quan.suning.com/getSysTime.do", cjson_time_info);
            break;
        case HTTP_GET_WEATHER:
            http_weather_get();
            break;
        case HTTP_GET_CITY:
            perform_http_get("http://pv.sohu.com/cityjson?ie=utf-8", cjson_city_info);
            break;
        default:
            break;
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void ds_http_request_init(void)
{
	http_request_event_queue = xQueueCreate(10, sizeof(HTTP_REQUEST_TYPE_E));
    xTaskCreate(&http_request_task, "http_request_task", 4096, NULL, 5, NULL);
}

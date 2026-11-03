#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

#include "http_request_rules.h"
#include "ui_weatherpage.h"
#include "nvs.h"
#include "system_data.h"

static const char *TAG = "HTTP_CLIENT";

static cJSON *parse_json_response(const char *text)
{
    // Some endpoints wrap JSON in a JavaScript assignment.
    const char *json = strchr(text, '{');
    return json ? cJSON_Parse(json) : NULL;
}

static void cjson_time_info(char *text)
{
    cJSON *root = parse_json_response(text);
    cJSON *time = cJSON_GetObjectItem(root, "sysTime1");
    if (cJSON_IsString(time) && strlen(time->valuestring) >= 14)
    {
        const char *value = time->valuestring;
        ESP_LOGI(TAG, "sysTime:%s", value);
        uint8_t hour = (value[8] - '0') * 10 + value[9] - '0';
        uint8_t minute = (value[10] - '0') * 10 + value[11] - '0';
        uint8_t second = (value[12] - '0') * 10 + value[13] - '0';
        update_system_time(hour, minute, second);
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

static void cjson_weather_info(char *text)
{
    cJSON *root = parse_json_response(text);
    cJSON *results = cJSON_GetObjectItem(root, "results");
    cJSON *item = cJSON_GetArrayItem(results, 0);
    cJSON *location = cJSON_GetObjectItem(item, "location");
    cJSON *now = cJSON_GetObjectItem(item, "now");
    if (cJSON_IsObject(location) && cJSON_IsObject(now))
    {
        update_weather_value(location, "name", VALUE_CITY, "city");
        update_weather_value(now, "text", VALUE_WEATHER, "weather");
        update_weather_value(now, "code", VALUE_CODE, "code");
        update_weather_value(now, "temperature", VALUE_TEMP, "temperatur");
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
    cJSON *root = parse_json_response(text);
    log_json_string(root, "cname", "name");
    log_json_string(root, "cid", "cid");
    cJSON_Delete(root);
}

static int gen_time_url(char *url, size_t size)
{
    return snprintf(url, size, "http://quan.suning.com/getSysTime.do");
}

static int gen_weather_url(char *url, size_t size)
{
    char *city = ds_nvs_read_city();
    int length = snprintf(url, size,
                          "http://api.seniverse.com/v3/weather/now.json"
                          "?key=SmazqPcltzTft-X3v&location=%s&language=zh-Hans&unit=c",
                          city ? city : "guangzhou");
    free(city);
    return length;
}

static int gen_city_url(char *url, size_t size)
{
    return snprintf(url, size, "http://pv.sohu.com/cityjson?ie=utf-8");
}

static const http_request_rule_t http_request_rules[] = {
    [HTTP_GET_TIME] = {.gen_url = gen_time_url, .parse_func = cjson_time_info},
    [HTTP_GET_WEATHER] = {.gen_url = gen_weather_url, .parse_func = cjson_weather_info},
    [HTTP_GET_CITY] = {.gen_url = gen_city_url, .parse_func = cjson_city_info},
};

const http_request_rule_t *ds_http_request_rule_get(HTTP_REQUEST_TYPE_E type)
{
    if ((unsigned)type >= sizeof(http_request_rules) / sizeof(http_request_rules[0]))
    {
        return NULL;
    }
    return &http_request_rules[type];
}

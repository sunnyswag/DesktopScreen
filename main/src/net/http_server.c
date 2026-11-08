/* Configuration web server: serves the embedded setting.html page and
 * receives the settings it posts back. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "cJSON.h"
#include "esp_http_server.h"

#include "http_server.h"
#include "system_data.h"
#include "nvs.h"
#include "wifi_ap_sta.h"
#include "pwm.h"
#include "ui_page_manage.h"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE 8192

struct http_server_data
{
    /* Scratch buffer for receiving request bodies */
    char scratch[SCRATCH_BUFSIZE];
};

static const char *TAG = "http_server";

/* Files embedded in flash via EMBED_FILES in main/CMakeLists.txt */
extern const unsigned char setting_html_start[] asm("_binary_setting_html_start");
extern const unsigned char setting_html_end[] asm("_binary_setting_html_end");
extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");

static esp_err_t send_embedded_file(httpd_req_t *req, const char *type,
                                    const unsigned char *start, const unsigned char *end)
{
    httpd_resp_set_type(req, type);
    return httpd_resp_send(req, (const char *)start, end - start);
}

/* Handler to respond with the settings page */
static esp_err_t setting_page_get_handler(httpd_req_t *req)
{
    return send_embedded_file(req, "text/html", setting_html_start, setting_html_end);
}

/* Browsers GET /favicon.ico on their own since the page declares no icon */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    return send_embedded_file(req, "image/x-icon", favicon_ico_start, favicon_ico_end);
}

static esp_err_t receive_request_body(httpd_req_t *req, char **body)
{
    size_t total_len = req->content_len;
    char *buf = ((struct http_server_data *)req->user_ctx)->scratch;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    size_t cur_len = 0;
    while (cur_len < total_len)
    {
        int received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    *body = buf;
    return ESP_OK;
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* A required string field and where to store its value. The stored pointer
 * points into the parsed JSON, so it stays valid until the root is deleted. */
typedef struct
{
    const char *name;
    char **value;
} config_field_t;

static cJSON *read_config_json(httpd_req_t *req, const char *message, const config_field_t fields[],
                               size_t field_count)
{
    char *body;
    if (receive_request_body(req, &body) != ESP_OK)
    {
        return NULL;
    }
    printf("recived data length is :%u\n%s\r\n%s\r\n", (unsigned int)req->content_len, body,
           message);
    cJSON *root = cJSON_Parse(body);
    if (!cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid configuration JSON");
        return NULL;
    }

    for (size_t i = 0; i < field_count; ++i)
    {
        const cJSON *item = cJSON_GetObjectItem(root, fields[i].name);
        if (!cJSON_IsString(item))
        {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing configuration field");
            return NULL;
        }
        *fields[i].value = item->valuestring;
    }
    return root;
}

static esp_err_t finish_config_request(httpd_req_t *req, cJSON *root)
{
    send_beep_event(BEEP_SHORT_500MS);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

static esp_err_t send_wifi_handler(httpd_req_t *req)
{
    char *ssid, *psw;
    const config_field_t fields[] = {
        {"wifi_name", &ssid},
        {"wifi_code", &psw},
    };
    cJSON *root = read_config_json(req, "wifi data recived!", fields, ARRAY_SIZE(fields));
    if (root == NULL)
    {
        return ESP_FAIL;
    }
    ds_nvs_save_wifi_info(ssid, psw);
    print_system_data_wifi_info();
    ds_wifi_send_event(AP_STA_UPDATE);
    return finish_config_request(req, root);
}

static esp_err_t send_tomato_handler(httpd_req_t *req)
{
    char *work_time, *rest_time, *time_count;
    const config_field_t fields[] = {
        {"work_time", &work_time},
        {"rest_time", &rest_time},
        {"time_count", &time_count},
    };
    cJSON *root = read_config_json(req, " tomato time data recived!", fields, ARRAY_SIZE(fields));
    if (root == NULL)
    {
        return ESP_FAIL;
    }
    set_system_data_tomato_info(work_time, rest_time, time_count);
    return finish_config_request(req, root);
}

static esp_err_t send_city_handler(httpd_req_t *req)
{
    char *city;
    const config_field_t fields[] = {
        {"city", &city},
    };
    cJSON *root = read_config_json(req, " city data recived!", fields, ARRAY_SIZE(fields));
    if (root == NULL)
    {
        return ESP_FAIL;
    }
    set_system_data_city_info(city);
    return finish_config_request(req, root);
}

static esp_err_t send_back_handler(httpd_req_t *req)
{
    char *body;
    if (receive_request_body(req, &body) != ESP_OK)
    {
        return ESP_FAIL;
    }
    ds_ui_page_manage_send_action(PAGE_TYPE_MEMU);
    return finish_config_request(req, NULL);
}

static void register_server_uri(httpd_handle_t server, const char *uri, httpd_method_t method,
                                esp_err_t (*handler)(httpd_req_t *req),
                                struct http_server_data *server_data)
{
    httpd_uri_t config = {
        .uri = uri,
        .method = method,
        .handler = handler,
        .user_ctx = server_data,
    };
    httpd_register_uri_handler(server, &config);
}

static esp_err_t start_http_server(void)
{
    static struct http_server_data *server_data = NULL;

    if (server_data)
    {
        ESP_LOGE(TAG, "HTTP server already started");
        return ESP_ERR_INVALID_STATE;
    }

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct http_server_data));
    if (!server_data)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        return ESP_FAIL;
    }

    register_server_uri(server, "/", HTTP_GET, setting_page_get_handler, server_data);
    register_server_uri(server, "/favicon.ico", HTTP_GET, favicon_get_handler, server_data);
    register_server_uri(server, "/wifi_data", HTTP_POST, send_wifi_handler, server_data);
    register_server_uri(server, "/tomato_data", HTTP_POST, send_tomato_handler, server_data);
    register_server_uri(server, "/city_data", HTTP_POST, send_city_handler, server_data);
    register_server_uri(server, "/back", HTTP_POST, send_back_handler, server_data);

    return ESP_OK;
}

void ds_http_server_init()
{
    ESP_ERROR_CHECK(start_http_server());
}

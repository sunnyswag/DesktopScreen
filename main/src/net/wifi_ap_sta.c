#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "wifi_ap_sta.h"
#include "system_data.h"
#include "pwm.h"
#include "ui_page_manage.h"

#define TAG "wifi_ap_sta"

/* Retry right away this many times, then once every STA_SLOW_RETRY_MS.
 * Each attempt scans other channels, which the soft-AP shares the radio
 * with, so nonstop retries make the settings hotspot unstable. */
#define STA_FAST_RETRY_MAX 5
#define STA_SLOW_RETRY_MS (30 * 1000)

static esp_netif_t *sta;
static esp_netif_t *ap;

static QueueHandle_t wifi_event_queue;
static TimerHandle_t sta_retry_timer;
static int sta_retry_count;

/* Start a fresh round of connection attempts, if any Wi-Fi is saved */
static void sta_connect(void)
{
    xTimerStop(sta_retry_timer, 0);
    sta_retry_count = 0;
    set_wifi_sta_status(WIFI_STA_MODE_INIT);
    if (has_wifi_info())
    {
        esp_wifi_connect();
    }
}

static void sta_retry_timer_cb(TimerHandle_t timer) { esp_wifi_connect(); }

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data)
{
    ESP_LOGI(TAG, "event_base = %s event_id = %" PRId32, event_base, event_id);
    // STA event
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        sta_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (sta_retry_count < STA_FAST_RETRY_MAX)
        {
            sta_retry_count++;
            set_wifi_sta_status(WIFI_STA_MODE_INIT);
            ESP_LOGI(TAG, "retry to connect to the AP (%d/%d)", sta_retry_count,
                     STA_FAST_RETRY_MAX);
            esp_wifi_connect();
        }
        else
        {
            set_wifi_sta_status(WIFI_STA_MODE_CONNECT_FAIL);
            ESP_LOGW(TAG, "connect failed, retry in %d s", STA_SLOW_RETRY_MS / 1000);
            xTimerStart(sta_retry_timer, 0);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "connect success ! got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        sta_retry_count = 0;
        send_beep_event(BEEP_SHORT_500MS);
        ds_ui_page_manage_send_action(PAGE_TYPE_MEMU);
        set_wifi_sta_status(WIFI_STA_MODE_CONNECT_SUCCESS);
    }

    // AP event
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        set_wifi_ap_status(WIFI_AP_MODE_CONNECT);
        send_beep_event(BEEP_SHORT_500MS);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        set_wifi_ap_status(WIFI_AP_MODE_DISCONNECT);
    }
}

/* Point the station at the saved Wi-Fi. With none saved it stays blank and
 * sta_connect() doesn't try to connect. */
static esp_err_t apply_sta_config(void)
{
    wifi_config_t sta_config = {
        .sta =
            {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {.capable = true, .required = false},
            },
    };
    if (has_wifi_info())
    {
        const SYSTRM_DATA_T data = get_system_data();
        memcpy(sta_config.sta.ssid, data.setting_ssid, MAX_SETTING_SSID_LEN);
        memcpy(sta_config.sta.password, data.setting_psw, MAX_SETTING_PSW_LEN);
    }
    /* The settings come from the user, so log a rejection instead of aborting */
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set station config: %s", esp_err_to_name(err));
    }
    return err;
}

static void ds_wifi_ap_sta_start(void)
{
    sta = esp_netif_create_default_wifi_sta();
    ap = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_config = {
        .ap = {.ssid = CONFIG_ESP_AP_WIFI_SSID,
               .ssid_len = strlen(CONFIG_ESP_AP_WIFI_SSID),
               .channel = CONFIG_ESP_AP_WIFI_CHANNEL,
               .password = CONFIG_ESP_AP_WIFI_PASSWORD,
               .max_connection = CONFIG_ESP_AP_MAX_STA_CONN,
               .authmode = WIFI_AUTH_OPEN},
    };
    apply_sta_config();
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/* Switch the station to newly saved Wi-Fi. A connected station must
 * disconnect before esp_wifi_connect() can join another AP. */
static void ds_wifi_ap_sta_update_info(void)
{
    esp_wifi_disconnect();
    if (apply_sta_config() == ESP_OK)
    {
        sta_connect();
    }
}

static void ds_wifi_ap_sta_stop(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP STOP");
    xTimerStop(sta_retry_timer, portMAX_DELAY);
    /* Unregister first so the disconnect caused by stopping isn't retried */
    ESP_ERROR_CHECK(
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler));
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    esp_netif_destroy_default_wifi(sta);
    esp_netif_destroy_default_wifi(ap);
    sta = NULL;
    ap = NULL;
}

static void wifi_net_task(void *arg)
{
    for (;;)
    {
        WIFI_SET_EVENT_E evt;
        xQueueReceive(wifi_event_queue, &evt, portMAX_DELAY);
        const bool is_open = get_is_ap_sta_open();
        ESP_LOGI(TAG, "get wifi set event %d (open=%d)", evt, is_open);
        switch (evt)
        {
        case AP_STA_START:
            if (is_open)
            {
                break;
            }
            set_is_ap_sta_open(true);
            set_wifi_sta_status(WIFI_STA_MODE_INIT);
            set_wifi_ap_status(WIFI_AP_MODE_DISCONNECT);
            ds_wifi_ap_sta_start();
            break;
        case AP_STA_UPDATE:
            /* While Wi-Fi is off, the next start reads the saved settings */
            if (is_open)
            {
                ds_wifi_ap_sta_update_info();
            }
            break;
        case AP_STA_STOP:
            if (!is_open)
            {
                break;
            }
            set_is_ap_sta_open(false);
            ds_wifi_ap_sta_stop();
            break;
        default:
            break;
        }
    }
}

void ds_wifi_send_event(WIFI_SET_EVENT_E event)
{
    WIFI_SET_EVENT_E evt;
    evt = event;
    xQueueSend(wifi_event_queue, &evt, 0);
}

void ds_wifi_ap_sta_init()
{
    wifi_event_queue = xQueueCreate(10, sizeof(WIFI_SET_EVENT_E));
    sta_retry_timer = xTimerCreate("sta_retry", pdMS_TO_TICKS(STA_SLOW_RETRY_MS), pdFALSE, NULL,
                                   sta_retry_timer_cb);
    xTaskCreate(wifi_net_task, "wifi_net_task", 4096, NULL, 10, NULL);
}

/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "ds_test.h"
#include "ds_wifi_scan.h"
#include "ds_ft6336.h"
#include "ds_screen.h"
#include "ds_spi.h"
#include "ds_wifi_ap.h"
#include "ds_wifi_sta.h"
#include "ds_http_server.h"
#include "ds_system_data.h"
#include "ds_nvs.h"
#include "ds_https_request.h"
#include "ds_http_request.h"
#include "ds_timer.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "ds_spiffs.h"
#include "ds_ui_timepage.h"
#include "ds_ui_page_manage.h"
#include "ds_ui_weatherpage.h"
#include "ds_ui_wordpage.h"
#include "ds_ui_tomatopage.h"
#include "ds_ui_fans.h"
#include "ds_font.h"
#include "ds_paint.h"


#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

static void ap_net_task_manage(void* arg)
{
    for(;;) {
        if((check_system_data_wifi_info() == SETTING_DATA_INIT || get_setting_status()== SETTING_ING)&&
                get_wifi_status() != WIFI_MODE_AP_RUNING){
            if(get_wifi_status() == WIFI_MODE_STA_RUNING)
                ds_wifi_sta_stop();
            if(get_setting_status() == SETTING_ING)
                set_setting_status(SETTING_WAIT);
            set_wifi_status(WIFI_MODE_AP_RUNING);
            ds_wifi_ap_start();
            ds_http_server_init();
            printf("ap start\n");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("ap task run \n");
    }
}

static void sta_net_task_manage(void* arg)
{
    for(;;) {
        //获取到wifi ssid和密码，且不在设置模式
        if(check_system_data_wifi_info() == SETTING_DATA_HAS_WIFI_INFO &&
            get_wifi_status() != WIFI_MODE_STA_RUNING && get_setting_status() != SETTING_WAIT){
            if(get_wifi_status() == WIFI_MODE_AP_RUNING)
                ds_wifi_ap_stop();
            set_wifi_status(WIFI_MODE_STA_RUNING);
            ds_wifi_sta_start();
            printf("sta start\n");
        }

        //退出设置页面
        if(get_setting_status() == SETTING_FINISH){
            set_setting_status(SETTING_NULL);
            if(PAGE_TYPE_SETTING == ds_ui_get_now_show_page()){
                ds_ui_page_manage_send_action(PAGE_TYPE_MEMU);
                if(get_wifi_status() == WIFI_MODE_AP_RUNING)
                    ds_wifi_ap_stop();
                set_wifi_status(WIFI_MODE_STA_RUNING);
                ds_wifi_sta_start();
                printf("sta start\n");
            }
        }
        //联网成功
        if(get_net_status() == NET_CONNECT  && get_setting_status() != SETTING_WAIT){
            if(PAGE_TYPE_SETTING == ds_ui_get_now_show_page()){
                ds_ui_page_manage_send_action(PAGE_TYPE_MEMU);
				ds_http_set_type(HTTP_GET_FANS);
			    ds_http_set_type(HTTP_GET_TIME);
				// ds_http_set_type(HTTP_GET_CITY);
            	ds_http_set_type(HTTP_GET_WEATHER);
				ds_http_set_type(HTTP_GET_FANS);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("sta task run \n");
    }
}


void app_main(void)
{
    printf("----- app start! -----\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
            CHIP_NAME,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ds_system_data_init();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //屏幕、TP外设初始化相关
    init_ft6336();
    init_screen_interface();
    //屏幕显示数据初始化
    ds_ui_timepage_init();
    ds_ui_page_manage_init();
    ds_ui_weather_init();
    ds_ui_wordpage_init();
    ds_ui_wordpage_init();
    ds_ui_tomatopage_init();
    ds_ui_fans_init();

    ds_timer_init();
    // ds_screen_init();

    //显示主页
    ds_ui_page_manage_send_action(PAGE_TYPE_SETTING);

    //网络数据读取
    // char *ssid="Kang";
    // char *psw="555883200";
    // set_system_data_wifi_info(ssid,strlen(ssid),psw,strlen(psw));
    // ds_nvs_save_wifi_info();
    // ds_nvs_read_wifi_info();
    ds_http_request_init();
    // ds_https_request_init();

    //热点扫描
    // ds_wifi_scan_start();


    xTaskCreate(ap_net_task_manage, "ap_net_task_manage", 4096, NULL, 10, NULL);
    xTaskCreate(sta_net_task_manage, "sta_net_task_manage", 4096, NULL, 10, NULL);
    

    for(;;){
        // screen_spi_test();
        printf("app run \n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

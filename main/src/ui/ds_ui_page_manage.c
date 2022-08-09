
#include "ds_ui_page_manage.h"
#include "ds_ui_mainpage.h"
#include "ds_ui_timepage.h"
#include "ds_ui_wordpage.h"
#include "ds_screen.h"
#include "ds_ui_weatherpage.h"
#include "ds_ui_tomatopage.h"
#include "ds_http_request.h"
#include "ds_ui_fans.h"

static const char *TAG = "DS_UI_PAGE_MANAGE";

PAGE_MANAGE_T g_page_manage;

typedef struct
{
	TP_ACTION_E key;
	uint8_t touch_x;
	uint8_t touch_y;
	PAGE_TYPE_E action;
}UI_EVENT_T;

xQueueHandle ui_event_queue;

void ds_ui_page_manage_send_event(TP_ACTION_E key,uint8_t touch_x,uint8_t touch_y){
	UI_EVENT_T evt;
	evt.key = key;
	evt.touch_x = touch_x;
	evt.touch_y = touch_y;
	xQueueSendFromISR(ui_event_queue, &evt, NULL);
}

void ds_ui_page_manage_send_action(PAGE_TYPE_E action){
	UI_EVENT_T evt;
	evt.action = action;
	xQueueSendFromISR(ui_event_queue, &evt, NULL);
}

static void ui_page_evt_task(void *arg)
{
    while (1) {
        UI_EVENT_T evt;
        xQueueReceive(ui_event_queue, &evt, portMAX_DELAY);
        ESP_LOGI(TAG, "ui_page_evt_task %d",evt.key);
		if(evt.action == PAGE_TYPE_MEMU){
			g_page_manage.now_show_page = PAGE_TYPE_MEMU;
			ds_screen_display_main();
		}else if(evt.action == PAGE_TYPE_SETTING){
			g_page_manage.now_show_page = PAGE_TYPE_SETTING;
			// ds_ui_wordpage_show(0);
			ds_screen_setting();
		}
		if(g_page_manage.now_show_page == PAGE_TYPE_MEMU){
			if(evt.key == TP_ACTION_SHORT){
				if(evt.touch_x < 75){
					if(evt.touch_y < 75){
						//时钟
						g_page_manage.now_show_page = PAGE_TYPE_TIME;
						// ds_screen_clean_white();
						ds_ui_timepage_show_time_init();
						// ds_ui_timepage_show_time(0);
					}else{
						//单词
						g_page_manage.now_show_page = PAGE_TYPE_WORD;
						// ds_screen_clean_white();
						ds_ui_wordpage_show(0);
					}
				}else{
					if(evt.touch_y < 75){
						//天气
						g_page_manage.now_show_page = PAGE_TYPE_WEATHER;
						// ds_screen_clean_white();
						ds_ui_weather_show(0);
					}else{
						//番茄时钟
						g_page_manage.now_show_page = PAGE_TYPE_TOMATO;
						// ds_screen_clean_white();
						ds_ui_tomatopage_show_init(0);
					}
				}
			}else if(evt.key == TP_ACTION_LONG){
	 			set_setting_status(SETTING_ING);
				g_page_manage.now_show_page = PAGE_TYPE_SETTING;
				ds_screen_setting();
			}else if(evt.key == TP_ACTION_MOVE_LEFT){
				g_page_manage.now_show_page = PAGE_TYPE_FANS;
				ds_ui_fans_show_init(0);
			}
		}else if(g_page_manage.now_show_page != PAGE_TYPE_MEMU){
			if(g_page_manage.now_show_page == PAGE_TYPE_FANS){
				if(evt.key == TP_ACTION_MOVE_LEFT){
					g_page_manage.now_show_page = PAGE_TYPE_FANS;
					ds_ui_fans_show_init(1);
				}
			}else if(g_page_manage.now_show_page == PAGE_TYPE_WORD){
				if(evt.key == TP_ACTION_MOVE_LEFT){
					g_page_manage.now_show_page = PAGE_TYPE_WORD;
					ds_ui_wordpage_show(1);
				}
			}
			if(evt.key == TP_ACTION_MOVE_RIGHT){
				g_page_manage.now_show_page = PAGE_TYPE_MEMU;
				ds_screen_display_main();
			}
		} 
		
		
    }
}

PAGE_TYPE_E ds_ui_get_now_show_page(){
	return g_page_manage.now_show_page;
}

void ds_ui_page_manage_init(){
	g_page_manage.now_show_page = PAGE_TYPE_MEMU;
	ui_event_queue = xQueueCreate(10, sizeof(UI_EVENT_T));
    xTaskCreate(ui_page_evt_task, "ui_page_evt_task", 4096, NULL, 5, NULL);
}



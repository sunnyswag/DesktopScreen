
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "screen.h"
#include "ui_mainpage.h"
#include "system_data.h"
#include "spi.h"

#include "data_page.h"
#include "wifi_ap_sta.h"


void ds_screen_display_main(){
	if(get_wifi_sta_status() == WIFI_STA_MODE_CONNECT_SUCCESS){
		ds_screen_full_display_bydata(gImage_main_page_wifi);
	}else
		ds_screen_full_display_bydata(gImage_main_page);
}  

void ds_screen_setting(){
	ds_screen_full_display_bydata(gImage_setting_page);
	
}




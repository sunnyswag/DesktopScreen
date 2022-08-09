
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_ui_mainpage.h"
#include "ds_system_data.h"
#include "ds_spi.h"

#include "ds_data_page.h"

void ds_ui_back_main_page(void){
    unsigned int i;
	for(i=0;i<2888;i++){
		spi_send_data(~gImage_main_page[i]);  
	} 
}

void ds_screen_display_main(){
	ds_screen_full_display_bydata(ds_screen_full_display_data,gImage_main_page);
}  

void ds_screen_setting(){
	ds_screen_full_display_bydata(ds_screen_full_display_data,gImage_setting_page);
	
}




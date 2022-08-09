
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_spi.h"
#include "ds_ui_page_manage.h"
#include "ds_data_icon.h"
#include "ds_ui_tomatopage.h"
#include "ds_system_data.h"
#include "ds_data_num.h"

typedef enum{
    TOMATO_WORK = 0,
    TOMATO_REST,
}TOMATO_PAGE_STATUS_E;

typedef struct 
{
    uint8_t init_status;
    uint8_t updateing;
    uint8_t second;
    uint8_t minute;
    uint8_t work_time;
    uint8_t rest_time;
    uint8_t times; //重复次数
    uint8_t second_last;
    uint8_t minute_last;
    uint8_t work_status;
}TOMATO_PAGE_T;

TOMATO_PAGE_T g_tomato_page;


static void ds_ui_tomatopage_num_set(const uint8_t *data){
	unsigned int i;
	for(i=0;i<192;i++){	
		    spi_send_data(data[i]);
	}
}

static void ds_ui_tomatopage_num_clear(const uint8_t *data)
{
	unsigned int i;
    if(g_tomato_page.init_status == 0){
        for(i=0;i<192;i++){
            spi_send_data(0x00);  
        }  
    }else{
        for(i=0;i<192;i++)	     {
		    spi_send_data(data[i]);  
	    }  
    }
}

void ds_ui_tomatopage_show_init(){
    g_tomato_page.init_status = 0;
    g_tomato_page.updateing = 1;
    int num_size = 32;
    int num_size_y = 48;
	int vertical = 35; //垂直位置
    int horizontal = 8;  //水平位置
    int now_index;

    g_tomato_page.minute_last = g_tomato_page.minute;
    g_tomato_page.second_last = g_tomato_page.second;
    
    ds_screen_partial_data_init();

    //小时 十位
    now_index = g_tomato_page.minute / 10;
    ds_screen_partial_data_add(horizontal+0,horizontal+num_size,vertical,vertical+num_size_y,gImage_num_size48[now_index]);
    //小时 个位
    now_index = g_tomato_page.minute % 10;
    ds_screen_partial_data_add(horizontal+num_size,horizontal+num_size*2,vertical,vertical+num_size_y,gImage_num_size48[now_index]);
    //首次刷新才显示 :号
    ds_screen_partial_data_add(horizontal+num_size*2,horizontal+num_size*2+8,vertical,vertical+num_size_y,gImage_time_symbol);

    //分钟 十位
    now_index = g_tomato_page.second / 10;
    ds_screen_partial_data_add(horizontal+num_size*2+8,horizontal+num_size*3+8,vertical,vertical+num_size_y,gImage_num_size48[now_index]);
    // //分钟 个位
    now_index = g_tomato_page.second % 10;
    ds_screen_partial_data_add(horizontal+num_size*3+8,horizontal+num_size*4+8,vertical,vertical+num_size_y,gImage_num_size48[now_index]);

    //图标
    num_size = 56;
    num_size_y = 56;
    vertical = 90;
    horizontal = 45;
    if(g_tomato_page.work_status == TOMATO_WORK)
        ds_screen_partial_data_add(horizontal,horizontal+num_size,vertical,vertical+num_size_y,gImage_work);
    else
        ds_screen_partial_data_add(horizontal,horizontal+num_size,vertical,vertical+num_size_y,gImage_rest);


    //返回
    //size 36*36
    num_size = 36;
    vertical = 0;
    ds_screen_partial_data_add(0,num_size,vertical,vertical+num_size,gImage_back);
    
    ds_screen_partial_data_copy();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    g_tomato_page.init_status = 1;
    g_tomato_page.updateing = 0;
}


void ds_ui_tomatopage_show(uint8_t type){
    if(g_tomato_page.updateing == 1)
        return ;

    printf("---time %d:%d",g_tomato_page.minute_last,g_tomato_page.second_last);

    g_tomato_page.updateing = 1;
    g_tomato_page.init_status = type;
    int num_size = 32;
    int num_size_y = 48;
	int vertical = 35; //垂直位置
    int horizontal = 8;  //水平位置
    int last_index,now_index;

    //小时 十位
    last_index = g_tomato_page.minute_last / 10;
    now_index = g_tomato_page.minute / 10;
    if(last_index != now_index)
        ds_screen_partial_display_bydata(horizontal+0,horizontal+num_size,vertical,vertical+num_size_y,ds_ui_tomatopage_num_clear,gImage_num_size48[last_index],ds_ui_tomatopage_num_set,gImage_num_size48[now_index]);
    // //小时 个位
    last_index = g_tomato_page.minute_last % 10;
    now_index = g_tomato_page.minute % 10;
    if(last_index != now_index)
        ds_screen_partial_display_bydata(horizontal+num_size,horizontal+num_size*2,vertical,vertical+num_size_y,ds_ui_tomatopage_num_clear,gImage_num_size48[last_index],ds_ui_tomatopage_num_set,gImage_num_size48[now_index]);

    //分钟 十位 10 *8 = 80  14*8 112
    last_index = g_tomato_page.second_last / 10;
    now_index = g_tomato_page.second / 10;
    if(last_index != now_index)
      ds_screen_partial_display_bydata(horizontal+num_size*2+8,horizontal+num_size*3+8,vertical,vertical+num_size_y,ds_ui_tomatopage_num_clear,gImage_num_size48[last_index],ds_ui_tomatopage_num_set,gImage_num_size48[now_index]);

    // //分钟 个位
    last_index = g_tomato_page.second_last % 10;
    now_index = g_tomato_page.second % 10;
    if(last_index != now_index)
      ds_screen_partial_display_bydata(horizontal+num_size*3+8,horizontal+num_size*4+8,vertical,vertical+num_size_y,ds_ui_tomatopage_num_clear,gImage_num_size48[last_index],ds_ui_tomatopage_num_set,gImage_num_size48[now_index]);
        

    g_tomato_page.minute_last = g_tomato_page.minute;
    g_tomato_page.second_last = g_tomato_page.second;
    g_tomato_page.init_status = 1;
    g_tomato_page.updateing = 0;
}

void ds_ui_tomatopage_updatetime(){
    // printf("ds_ui_tomatopage_updatetime %d %d\n",g_tomato_page.minute,g_tomato_page.second);
    if(g_tomato_page.second <= 0){
        if(g_tomato_page.minute <= 0){
            //TODO finish
            g_tomato_page.work_status = TOMATO_REST;
            g_tomato_page.minute = g_tomato_page.rest_time;
            g_tomato_page.second = 0;
            if(ds_ui_get_now_show_page() == PAGE_TYPE_TOMATO)
                ds_ui_tomatopage_show_init();
        }else{
            g_tomato_page.minute --;
            g_tomato_page.second = 59;
        }
    }else{
        g_tomato_page.second --;
    }
    if(g_tomato_page.updateing == 0){
        if(ds_ui_get_now_show_page() == PAGE_TYPE_TOMATO)
            ds_ui_tomatopage_show(1);
    }
}

void ds_ui_tomatopage_time_set(uint8_t work_time,uint8_t rest_time,uint8_t times){
    g_tomato_page.work_time = work_time;
    g_tomato_page.rest_time = rest_time;
    g_tomato_page.times = times;
    g_tomato_page.minute = work_time;
    g_tomato_page.second = 0;
}

void ds_ui_tomatopage_init(){
    memset(&g_tomato_page,0,sizeof(TOMATO_PAGE_T));
    g_tomato_page.work_time = 20;
    g_tomato_page.rest_time = 5;
    g_tomato_page.times = 3;
    g_tomato_page.updateing = 0;
    g_tomato_page.minute = g_tomato_page.work_time;
    g_tomato_page.second = 0;
    g_tomato_page.work_status = TOMATO_WORK;
}
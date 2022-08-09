#ifndef _DS_SYSTEM_DATA_H_
#define _DS_SYSTEM_DATA_H_

#include <stdio.h>
#include <string.h>


#define MAX_SETTING_SSID_LEN 32
#define MAX_SETTING_PSW_LEN 64
#define MAX_SETTING_CITY_LEN 15

typedef enum{
    SETTING_DATA_INIT = 0,
    SETTING_DATA_HAS_WIFI_INFO,
}SETTING_DATA_E;

typedef enum{
    TP_ACTION_NULL = 0,
    //短按
    TP_ACTION_SHORT,  
    //长按
    TP_ACTION_LONG,        
    //滑动
    TP_ACTION_MOVE_LEFT,
    TP_ACTION_MOVE_RIGHT,
    TP_ACTION_MOVE_UP,
    TP_ACTION_MOVE_DOWN,
}TP_ACTION_E;

typedef enum{
    NET_DISCONNECT =0,
    NET_CONNECT,
}NET_STATUS_E;

typedef enum{
    SETTING_NULL =0,
    SETTING_ING,
    SETTING_WAIT,
    SETTING_FINISH,
}SETTING_STATUS_E;

typedef enum{
    WIFI_MODE_INIT = 0,
    WIFI_MODE_AP_RUNING,
    WIFI_MODE_STA_RUNING,
}WIFI_MODE_E;

//触摸动作管理
typedef struct{
    int tp_interval_timecount;
    uint8_t status;
    uint16_t tp_start_x;
	uint16_t tp_start_y;
    uint16_t tp_stop_x;
	uint16_t tp_stop_y;
    TP_ACTION_E tp_action;
}TP_ACTION_MANAGE_T;

typedef struct
{
    char setting_ssid[MAX_SETTING_SSID_LEN];
    uint8_t setting_ssid_len;
    char setting_psw[MAX_SETTING_PSW_LEN];
    uint8_t setting_psw_len;
    char setting_city[MAX_SETTING_CITY_LEN];
    uint8_t setting_city_len;


    //番茄时钟 工作时间 休息时间 轮次
    uint8_t tomato_work_time;
    uint8_t tomato_rest_time;
    uint8_t tomato_time_count;

    TP_ACTION_MANAGE_T tp_action_manage;
    NET_STATUS_E net_status;

    WIFI_MODE_E wifi_status;
    SETTING_STATUS_E setting_status;

}SYSTRM_DATA_T;


void set_system_data_tomato_info(char * work_time,char * rest_time,char * count_time);
//////////////////WIFI INFO MANAGE //////////////////////////////

void set_system_data_wifi_info(char * p_ssid,char *p_psw);

void print_system_data_wifi_info(void);

void set_system_data_city_info(char * p_city);

SETTING_DATA_E check_system_data_wifi_info(void);

SYSTRM_DATA_T get_system_data(void);

void ds_system_data_init(void);

////////////////////TP ACTION MANAGE ////////////////////////////
void clear_tp_action(void);
uint8_t get_tp_action_status(void);
TP_ACTION_E get_tp_action(void);
void check_tp_action(void);
void set_tp_action_manage_stop_point(uint16_t x,uint16_t y);
void set_tp_action_manage_start_point(uint16_t x,uint16_t y);
void count_tp_action_manage_time(void);
void reset_tp_action_manage(void);

NET_STATUS_E get_net_status();
void set_net_status(NET_STATUS_E status);

WIFI_MODE_E get_wifi_status();
void set_wifi_status(WIFI_MODE_E status);

SETTING_STATUS_E get_setting_status();
void set_setting_status(SETTING_STATUS_E status);

#endif


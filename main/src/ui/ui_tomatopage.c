
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "screen.h"
#include "spi.h"
#include "ui_clock.h"
#include "ui_page_manage.h"
#include "data_icon.h"
#include "ui_tomatopage.h"
#include "system_data.h"
#include "data_num.h"
#include "nvs.h"

#include "pwm.h"

typedef enum
{
    // 工作倒计时
    TOMATO_TYPE_WORK = 0,
    // 休息倒计时
    TOMATO_TYPE_REST,
} TOMATO_PAGE_TYPE_E;

typedef enum
{
    // 开始倒计时
    TOMATO_COUNT_START = 0,
    // 停止倒计时
    TOMATO_COUNT_STOP,
    // 完成任务
    TOMATO_COUNT_FINISH,
} TOMATO_PAGE_STATUS_E;

typedef struct
{
    // 页面是否在更新
    bool is_updateing;

    // 番茄时钟设置的工作休息时间，循环次数
    uint8_t work_time;
    uint8_t rest_time;
    uint8_t times;

    // 运行时间&次数统计
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t timescount;

    // 缓存上次刷新数据
    uint8_t second_last;
    uint8_t minute_last;
    uint8_t hour_last;

    // 倒计时类型
    TOMATO_PAGE_TYPE_E work_type;
    // 当前页面工作状态
    TOMATO_PAGE_STATUS_E work_status;

    // 局部刷新次数
    uint8_t partial_update_time;
} TOMATO_PAGE_T;

// 200*200 像素屏幕适配，偏移值
int offset_v = 29;
int offset_h = 24;

TOMATO_PAGE_T g_tomato_page;

static void ds_ui_tomatopage_update()
{
    if (g_tomato_page.is_updateing)
        return;
    g_tomato_page.is_updateing = true;
    ds_ui_clock_update_time(8 + offset_h, 35 + offset_v, g_tomato_page.hour, g_tomato_page.minute);
    g_tomato_page.hour_last = g_tomato_page.hour;
    g_tomato_page.minute_last = g_tomato_page.minute;
    g_tomato_page.is_updateing = false;
}

void ds_ui_tomatopage_show_init()
{
    g_tomato_page.is_updateing = true;
    int num_size = 32;
    int num_size_y = 48;
    int vertical = 35 + offset_v;  // 垂直位置
    int horizontal = 8 + offset_h; // 水平位置
    g_tomato_page.hour_last = g_tomato_page.hour;
    g_tomato_page.minute_last = g_tomato_page.minute;

    ds_screen_partial_data_init();
    ds_ui_clock_add_time(horizontal, vertical, g_tomato_page.hour, g_tomato_page.minute);

    // 图标
    num_size = 56;
    num_size_y = 56;
    vertical = 90 + offset_v;
    horizontal = 45 + offset_h;
    if (g_tomato_page.work_status == TOMATO_COUNT_STOP)
    {
        // TODO 停止文字图-点击开始
        ds_screen_partial_data_add(horizontal, horizontal + num_size, vertical,
                                   vertical + num_size_y, gImage_click_start);
    }
    else if (g_tomato_page.work_status == TOMATO_COUNT_FINISH)
    {
        // TODO 完成文字图-恭喜-已完成
        ds_screen_partial_data_add(horizontal, horizontal + num_size, vertical,
                                   vertical + num_size_y, gImage_tomato_finish);
    }
    else
    {
        if (g_tomato_page.work_type == TOMATO_TYPE_WORK)
            ds_screen_partial_data_add(horizontal, horizontal + num_size, vertical,
                                       vertical + num_size_y, gImage_work);
        else
            ds_screen_partial_data_add(horizontal, horizontal + num_size, vertical,
                                       vertical + num_size_y, gImage_rest);
    }

    // 返回
    // size 36*36
    num_size = 36;
    vertical = 0;
    ds_screen_partial_data_add(0, num_size, vertical, vertical + num_size, gImage_back);
    ds_screen_partial_data_copy();
    g_tomato_page.is_updateing = false;
}

void ds_ui_tomatopage_start_toggle()
{
    if (g_tomato_page.work_status == TOMATO_COUNT_START)
    {
        g_tomato_page.work_status = TOMATO_COUNT_STOP;
    }
    else if (g_tomato_page.work_status == TOMATO_COUNT_STOP)
    {
        g_tomato_page.work_status = TOMATO_COUNT_START;
    }
    else if (g_tomato_page.work_status == TOMATO_COUNT_FINISH)
    {
        // 完成状态下点击，重新开始
        g_tomato_page.minute = g_tomato_page.work_time;
        g_tomato_page.second = 59;
        g_tomato_page.timescount = g_tomato_page.times;
        g_tomato_page.work_status = TOMATO_COUNT_START;
    }
    ds_ui_tomatopage_show_init();
}

void ds_ui_tomatopage_updatetime()
{
    if (g_tomato_page.work_status == TOMATO_COUNT_STOP ||
        g_tomato_page.work_status == TOMATO_COUNT_FINISH)
        return;
    if (g_tomato_page.second <= 0)
    {
        if (g_tomato_page.minute <= 0)
        {
            if (g_tomato_page.timescount > 0)
            {
                // TODO update
                g_tomato_page.timescount--;
                if (g_tomato_page.work_type == TOMATO_TYPE_WORK)
                {
                    g_tomato_page.work_type = TOMATO_TYPE_REST;
                    g_tomato_page.minute = g_tomato_page.rest_time;
                    g_tomato_page.second = 59;
                }
                else
                {
                    g_tomato_page.work_type = TOMATO_TYPE_WORK;
                    g_tomato_page.minute = g_tomato_page.work_time;
                    g_tomato_page.second = 59;
                }
            }
            else
            {
                send_beep_event(BEEP_LONG);
                // TODO finish
                g_tomato_page.work_status = TOMATO_COUNT_FINISH;
            }
            // 每轮刷新
            if (ds_ui_get_now_show_page() == PAGE_TYPE_TOMATO)
            {
                g_tomato_page.partial_update_time = 0;
                send_beep_event(BEEP_LONG);
                ds_ui_tomatopage_show_init();
                return;
            }
        }
        else
        {
            g_tomato_page.minute--;
            g_tomato_page.second = 59;
            if (ds_ui_get_now_show_page() == PAGE_TYPE_TOMATO)
            {
                // 局部刷新大于5次时，全局刷新
                if (g_tomato_page.partial_update_time > 5)
                {
                    g_tomato_page.partial_update_time = 0;
                    ds_ui_tomatopage_show_init();
                }
                else
                {
                    g_tomato_page.partial_update_time++;
                    ds_ui_tomatopage_update(true);
                }
            }
        }
    }
    else
    {
        g_tomato_page.second--;
    }
}

void ds_ui_tomatopage_time_set(uint8_t work_time, uint8_t rest_time, uint8_t times)
{
    ds_nvs_save_tomato_time(work_time, rest_time, times);
    g_tomato_page.work_time = work_time;
    g_tomato_page.rest_time = rest_time;
    g_tomato_page.times = times;
    g_tomato_page.minute = work_time;
    g_tomato_page.second = 59;
}

void ds_ui_tomatopage_init()
{
    memset(&g_tomato_page, 0, sizeof(TOMATO_PAGE_T));
    ds_nvs_read_tomato_time(&g_tomato_page.work_time, &g_tomato_page.rest_time,
                            &g_tomato_page.times);
    g_tomato_page.hour = 0;
    g_tomato_page.minute = g_tomato_page.work_time;
    g_tomato_page.second = 59;
    g_tomato_page.work_status = TOMATO_COUNT_STOP;
    g_tomato_page.work_type = TOMATO_TYPE_WORK;
    g_tomato_page.timescount = g_tomato_page.times;
}
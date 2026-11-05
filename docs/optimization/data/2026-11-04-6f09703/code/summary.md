# 代码量与复杂度

- 被测代码：提交 6f09703bd2ff464dff15b2d5db1f5525e513cd46
- 范围：main/app_main.c、main/src（不含 data）、main/include；工具 lizard（-l c）

## 总览

| 指标 | 数值 |
|---|---|
| 源文件数（.c/.h） | 55 |
| 物理行 | 5254 |
| 代码行 NLOC（去注释和空行） | 3966 |
| 注释行 | 612 |
| 函数数 | 180 |
| 平均圈复杂度 | 2.95 |
| 最大圈复杂度 | 29 |
| CCN > 10 的函数 | 8 |
| CCN > 15 的函数 | 5 |
| 平均函数长度（NLOC） | 16.8 |
| 最长函数（NLOC） | 86 |
| 长度 > 60 NLOC 的函数 | 7 |
| 重复代码率（%） | 0.9 |
| 重复代码块 | 1 |
| printf 调用 | 37 |
| ESP_LOGx 调用 | 71 |
| data 目录文件数 | 4 |
| data 目录行数 | 1703 |
| data 目录源码字节 | 123234 |

## 按模块

| 模块 | 文件 | 物理行 | NLOC | 注释行 | printf | ESP_LOGx |
|---|---|---|---|---|---|---|
| app_main.c | 1 | 258 | 198 | 33 | 9 | 0 |
| include | 29 | 674 | 456 | 62 | 0 | 0 |
| src/draw | 2 | 545 | 370 | 126 | 7 | 0 |
| src/driver | 2 | 414 | 320 | 44 | 5 | 0 |
| src/fs | 1 | 85 | 59 | 14 | 0 | 9 |
| src/hal | 6 | 791 | 554 | 145 | 4 | 4 |
| src/net | 5 | 1174 | 925 | 111 | 4 | 46 |
| src/system_data.c | 1 | 324 | 270 | 17 | 7 | 11 |
| src/ui | 8 | 989 | 814 | 60 | 1 | 1 |

## 圈复杂度最高的函数

| 函数 | 位置 | CCN | NLOC | 参数 |
|---|---|---|---|---|
| ds_ui_weather_show | main/src/ui/ui_weatherpage.c:51 | 29 | 86 | 1 |
| ui_page_evt_task | main/src/ui/ui_page_manage.c:81 | 20 | 86 | 1 |
| check_tp_action | main/src/system_data.c:177 | 17 | 84 | 0 |
| Paint_SetPixel | main/src/draw/paint.c:142 | 16 | 69 | 3 |
| count_position_ft6336 | main/src/driver/ft6336.c:56 | 16 | 39 | 1 |
| background_task | main/app_main.c:128 | 13 | 55 | 1 |
| upload_post_handler | main/src/net/http_server.c:322 | 11 | 76 | 1 |
| wifi_event_handler | main/src/net/wifi_ap_sta.c:29 | 11 | 38 | 4 |
| download_get_handler | main/src/net/http_server.c:233 | 10 | 63 | 1 |
| ds_ui_tomatopage_updatetime | main/src/ui/ui_tomatopage.c:150 | 10 | 62 | 0 |
| sleep_mode_init | main/app_main.c:56 | 8 | 57 | 0 |
| Paint_DrawString_CN | main/src/draw/paint.c:301 | 7 | 60 | 5 |

## 最长的函数

| 函数 | 位置 | NLOC | CCN |
|---|---|---|---|
| ds_ui_weather_show | main/src/ui/ui_weatherpage.c:51 | 86 | 29 |
| ui_page_evt_task | main/src/ui/ui_page_manage.c:81 | 86 | 20 |
| check_tp_action | main/src/system_data.c:177 | 84 | 17 |
| upload_post_handler | main/src/net/http_server.c:322 | 76 | 11 |
| Paint_SetPixel | main/src/draw/paint.c:142 | 69 | 16 |
| download_get_handler | main/src/net/http_server.c:233 | 63 | 10 |
| ds_ui_tomatopage_updatetime | main/src/ui/ui_tomatopage.c:150 | 62 | 10 |
| Paint_DrawString_CN | main/src/draw/paint.c:301 | 60 | 7 |

## 重复代码块（lizard -Eduplicate）

- main/src/net/wifi_ap_sta.c:83 ~ 95 ↔ main/src/net/wifi_ap_sta.c:112 ~ 123

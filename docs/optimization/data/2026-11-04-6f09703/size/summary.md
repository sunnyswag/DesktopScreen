# 静态资源评估

- 被测代码：提交 6f09703bd2ff464dff15b2d5db1f5525e513cd46

## 分区余量

| 项目 | 大小 | 余量 |
|---|---|---|
| app（DesktopScreen.bin） | 964.3 KB / 分区 1024.0 KB（94.2%） | 59.7 KB |
| bootloader | 25.4 KB | 2.6 KB |

| 分区 | 类型 | 偏移 | 大小 |
|---|---|---|---|
| nvs | data/nvs | 0x9000 | 24.0 KB |
| phy_init | data/phy | 0xF000 | 4.0 KB |
| factory | app/factory | 0x10000 | 1024.0 KB |
| storage | data/spiffs | 0x110000 | 308.0 KB |
| storage1 | 0x40/0x01 | 0x15D000 | 2048.0 KB |
| （未分配） | - | 0x35D000 | 652.0 KB |

## 内存段

| 段 | 已用 | 总量 | 占比 |
|---|---|---|---|
| Flash Code | 684616 B（.text 684616） | - | - |
| Flash Data | 178352 B（.rodata 178096，.appdesc 256） | - | - |
| IRAM | 103658 B（.text 102631，.vectors 1027） | 131072 | 79.1% |
| DRAM | 55360 B（.bss 34696，.data 20664） | 180736 | 30.6% |
| RTC FAST | 28 B（.force_fast 28） | 8192 | 0.3% |
| RTC SLOW | 24 B（.rtc_slow_reserved 24） | 8192 | 0.3% |

合并字符串池（日志/断言字符串，记在 esp_app_desc.c.obj 名下）：88.6 KB

## 按模块归类

| 归类 | Flash | DRAM | IRAM |
|---|---|---|---|
| WiFi/PHY 驱动与协议栈 | 262.7 KB | 18.2 KB | 35.5 KB |
| mbedTLS 加密/TLS | 114.4 KB | 0.4 KB | 0.0 KB |
| lwIP/网络接口 | 89.9 KB | 3.7 KB | 0.0 KB |
| HTTP 客户端/服务器 | 31.6 KB | 0.0 KB | 0.0 KB |
| C 运行库 | 82.7 KB | 1.1 KB | 1.4 KB |
| 存储（SPIFFS/NVS/VFS/分区） | 45.7 KB | 1.6 KB | 7.7 KB |
| 业务代码 main | 48.5 KB | 19.6 KB | 0.1 KB |
| 其他（FreeRTOS、系统、驱动、字符串池等） | 161.0 KB | 9.2 KB | 54.9 KB |

### 体积前 15 的库

| 库 | 合计 | Flash | DRAM | IRAM |
|---|---|---|---|---|
| libnet80211 | 146485 | 131507 | 9567 | 5411 |
| libesp_app_format | 91422 | 91405 | 17 | 0 |
| liblwip | 90309 | 86519 | 3790 | 0 |
| libc | 82046 | 81334 | 712 | 0 |
| libmbedcrypto | 80928 | 80787 | 141 | 0 |
| libmain | 69914 | 49680 | 20099 | 135 |
| libpp | 69092 | 44114 | 4099 | 20879 |
| libwpa_supplicant | 54476 | 53147 | 1329 | 0 |
| libphy | 46158 | 34110 | 3054 | 8994 |
| libspiffs | 25977 | 25965 | 12 | 0 |
| libmbedtls | 24124 | 23858 | 266 | 0 |
| libesp_hw_support | 21222 | 10748 | 429 | 9993 |
| libfreertos | 20417 | 2763 | 3841 | 13813 |
| libhal | 14592 | 5236 | 1949 | 7407 |
| libnvs_flash | 12510 | 12482 | 28 | 0 |

## main 组件各文件

| 文件 | Flash | DRAM |
|---|---|---|
| data_page.c.obj | 15000 | 0 |
| screen.c.obj | 819 | 10000 |
| setting.html.S.obj | 7884 | 0 |
| paint.c.obj | 1485 | 5024 |
| favicon.ico.S.obj | 6433 | 0 |
| http_server.c.obj | 3042 | 4 |
| data_icon.c.obj | 2344 | 192 |
| data_num.c.obj | 480 | 1956 |
| data_weather.c.obj | 0 | 2016 |
| wifi_ap_sta.c.obj | 1589 | 12 |
| system_data.c.obj | 1353 | 160 |
| nvs.c.obj | 996 | 4 |
| app_main.c.obj | 793 | 0 |
| http_request.c.obj | 691 | 4 |
| ui_wordpage.c.obj | 438 | 203 |
| http_request_rules.c.obj | 631 | 0 |
| ui_tomatopage.c.obj | 528 | 32 |
| timer.c.obj | 410 | 32 |
| font.c.obj | 239 | 305 |
| ft6336.c.obj | 506 | 24 |
| ui_weatherpage.c.obj | 476 | 48 |
| ui_timepage.c.obj | 494 | 9 |
| sntp.c.obj | 409 | 37 |
| gpio.c.obj | 412 | 5 |
| ui_page_manage.c.obj | 423 | 8 |
| spiffs.c.obj | 350 | 0 |
| ui_fans.c.obj | 326 | 16 |
| ui_clock.c.obj | 287 | 0 |
| i2c.c.obj | 275 | 0 |
| pwm.c.obj | 264 | 4 |
| spi.c.obj | 254 | 4 |
| ui_mainpage.c.obj | 49 | 0 |

## 图片数组（gImage_* 符号）

Flash 合计 21976 B，DRAM 合计 9152 B。除 gImage_last_page（paint.c 的整屏缓冲）外都定义在 main/src/data。

| 符号 | 大小 | 位置 |
|---|---|---|
| gImage_last_page | 5000 | DRAM .bss |
| gImage_main_page | 5000 | Flash（只读） |
| gImage_main_page_wifi | 5000 | Flash（只读） |
| gImage_setting_page | 5000 | Flash（只读） |
| gImage_num_weather_icon | 2016 | DRAM .data（Flash 另存初值） |
| gImage_num_size48 | 1920 | DRAM .data（Flash 另存初值） |
| gImage_num_size24 | 480 | Flash（只读） |
| gImage_TH | 392 | Flash（只读） |
| gImage_click_start | 392 | Flash（只读） |
| gImage_rest | 392 | Flash（只读） |
| gImage_tomato_finish | 392 | Flash（只读） |
| gImage_work | 392 | Flash（只读） |
| gImage_back | 180 | DRAM .data（Flash 另存初值） |
| gImage_fans | 128 | Flash（只读） |
| gImage_humidity | 128 | Flash（只读） |
| gImage_template | 128 | Flash（只读） |
| gImage_time_symbol | 36 | DRAM .data（Flash 另存初值） |

## 编译告警（去重）

共 5 条，其中项目代码 3 条。

- `main/src/draw/paint.c:322:13: warning: unused variable 'size' [-Wunused-variable]`
- `main/src/driver/screen.c:138:18: warning: unused variable 'i' [-Wunused-variable]`
- `main/src/net/wifi_ap_sta.c:32:16: warning: variable 's_retry_num' set but not used [-Wunused-but-set-variable]`
- `$IDF_PATH/components/driver/deprecated/driver/mcpwm.h:15:2: warning: #warning "legacy MCPWM driver is deprecated, please migrate to the new driver (include driver/mcpwm_prelude.h)" [-Wcpp]`
- `$IDF_PATH/components/driver/deprecated/driver/timer.h:16:2: warning: #warning "legacy timer group driver is deprecated, please migrate to driver/gptimer.h" [-Wcpp]`

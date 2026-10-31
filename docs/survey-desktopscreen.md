> 行号基于 DesktopScreen 原始提交 `e18432a`（清理前）。main 分支已删除死代码，行号已变。

核实完成，所有关键事实已对照原文。以下为最终交付的事实清单。

---

# DesktopScreen（ESP-IDF / ESP32）代码摸底事实清单

**执行说明**：本清单由 3 个只读 Explore 子代理分块通读全部源码（HAL/驱动、主流程/UI、网络/FS/字库/数据），我本人另行 grep 了全部 ESP-IDF/FreeRTOS API 调用建立权威清单，并亲自复核了以下文件的原文：app_main.c（全文）、ds_ui_page_manage.c（全文）、ds_ui_page_manage.h、ds_font.c（全文）、ds_system_data.h、ds_screen.c(1–330)、ds_timer.c（全文）、ds_paint.c(430–538)、ds_ui_tomatopage.c、ds_ui_timepage.c、ds_ui_mainpage.c、ds_ft6336.c、ds_gpio.c（全文）、partitions.csv、main/CMakeLists.txt、sdkconfig 关键项、data/ 数组声明，并用 grep 确认了全部“死代码无调用者”结论。数据数组**内容**、setting.html 行号、net 层部分细节来自子代理报告（抽查一致，未逐条核实）。因当前处于 plan 模式（只读），orchestrate 的 codex/zcode 脚本不可用，改用会话内 Explore subagent 承担同等读取工作。

**工程环境**：ESP32 经典款（`CONFIG_IDF_TARGET="esp32"`，sdkconfig）、IDF 5.4.1（`CONFIG_IDF_INIT_VERSION`，sdkconfig:250，子代理报告）、FreeRTOS HZ=100、主任务栈 3584、4MB Flash DIO、自定义分区表。总代码量 ~10148 行（含 .h）。

**分区表**（partitions.csv:3-7）：nvs @0x9000/0x6000；phy @0xf000/0x1000；factory @0x10000/0x100000；storage(spiffs) @0x110000/0x4D000；**storage1(type 0x40/subtype 0x01) @0x15D000/0x200000** 存字库。字库文件 `myFont1.bin`（仓库根）= **1,480,385 字节**。CMake 嵌入 `favicon.ico`、`setting.html`（EMBED_FILES）与 `server_root_cert.pem`、`server_root_cert_bilibili.pem`（EMBED_TXTFILES）（main/CMakeLists.txt:58-63）。顶层 components/ 目录为空，无自研组件。

---

## 1. 模块清单

（路径相对 `main/`；"(死)"= 编译但无任何调用者，见 §10）

| 文件 | 职责 | 对外（非 static）函数 file:line | 依赖 ds_ 模块 |
|---|---|---|---|
| app_main.c | 入口：全系统初始化、后台任务、浅睡管理死循环 | app_main:178 | 几乎全部（见 §5） |
| src/hal/ds_gpio.c | GPIO 配置；屏幕 CS/DC/RST/BUSY 读写；TP INT 的 ISR+队列+任务 | ds_touch_gpio_init:68、ds_touch_gpio_isr_remove:111、ds_touch_gpio_isr_add:115、ds_screen_gpio_init:119、ds_gpio_init:161(死)、set_screen_cs/dc/rst:165/169/173、get_screen_busy:177、set_touch_rst:181 | ds_system_data |
| src/hal/ds_i2c.c | 旧版 I2C 主机 API 封装（写寄存器地址→读/写） | i2c_master_read_slave:36、i2c_master_write_slave:62、i2c_master_init:83 | 无 |
| src/hal/ds_nvs.c | NVS 封装（WiFi/城市/番茄钟） | save_wifi_info:27、read_wifi_info:44、clean_wifi_info:77(死)、save_city:91、read_city:104、save_tomato_time:125、read_tomato_time:140、ds_nvs_test_func:159(死) | ds_system_data |
| src/hal/ds_pwm.c | 蜂鸣器 PWM（**MCPWM 旧驱动，非 LEDC**）+ 蜂鸣事件队列任务 | send_beep_event:68、send_beep_event_from_isr:73、ds_pwm_init:78 | 无 |
| src/hal/ds_spi.c | SPI 总线初始化 + 屏幕单字节命令/数据发送（软件 CS/DC） | spi_send_cmd:39、spi_send_data:55、screen_spi_init:79、screen_spi_test:106(死) | ds_gpio |
| src/hal/ds_timer.c | 硬件定时器组0 定时器0（10ms）+ 2 个消费任务 | timer_group0_isr:51、ds_timer_init:159 | ds_system_data、ds_ft6336、ds_ui_timepage、ds_ui_tomatopage |
| src/driver/ds_screen.c | SSD1681 200×200 驱动：初始化序列/全刷/局刷/合成缓冲 | deep_sleep:84、refresh:91、refresh_part:99、full_display_data:123、full_display:135、full_display_bydata:143、partial_display:151(死)、partial_display_bydata:204、partial_data_init:261、partial_data_add:269、partial_data_copy:304、init_screen_interface:316、clean_white:322(死)、ds_screen_init:328(死)＋demo 链 EPD_*:332-898(死) | ds_gpio、ds_spi（include 了 ds_data_image/ds_data_page 但未用） |
| src/driver/ds_ft6336.c | FT6336 初始化 + 轮询读触点 + 坐标缩放 | get_ft6336_touch_sta:122、init_ft6336:137 | ds_i2c、ds_gpio |
| src/fs/ds_spiffs.c | 仅挂载 SPIFFS | init_spiffs:24 | 无 |
| src/net/ds_wifi_ap_sta.c | **实际使用的** AP+STA 复合模式（事件队列+任务） | ds_wifi_send_event:171、ds_wifi_ap_sta_init:177 | ds_system_data、ds_pwm、ds_ui_page_manage |
| src/net/ds_http_server.c | 配网/设置 httpd 服务器（通配路由）+ SPIFFS 文件服务 | start_file_server:557、ds_http_server_init:652 | ds_spiffs、ds_system_data、ds_nvs、ds_wifi_ap_sta、ds_pwm、ds_ui_page_manage |
| src/net/ds_http_request.c | 出站 HTTP（esp_http_client）+ cJSON 解析 + 队列任务 | cjson_weather_info:110、cjson_fans_info:165(死)、http_time_get:458(废弃)、http_weather_get:478、http_fans_get:520(死)、http_city_get:550(结果未用)、ds_http_post:571(死)、ds_http_request_type:604、ds_http_request_all:610、ds_http_request_init:642 | ds_ui_timepage、ds_ui_fans、ds_ui_weatherpage、ds_nvs、ds_system_data |
| src/net/ds_sntp.c | SNTP 初始化、时区、同步回调更新系统时间 | get_system_time:28、time_sync_notification_cb:47、initialize_sntp:56 | ds_system_data |
| src/net/ds_wifi_scan.c (死) | WiFi 扫描示例，结果仅打印 | ds_wifi_scan_start:131、stop:136 | 无 |
| src/net/ds_wifi_ap.c (死) | 纯 SoftAP 旧模块 | wifi_init_softap:51、ds_wifi_ap_start:92、stop:103 | 无 |
| src/net/ds_wifi_sta.c (死) | 纯 STA 旧模块（含事件组） | wifi_init_sta:74、ds_wifi_sta_start:147、stop:158 | ds_system_data |
| src/net/ds_dns_server.c (死) | DNS 劫持+手写 web server，实现全被注释，两个入口为空函数 | dns_server_start:145(空)、web_server_start:330(空) | 无 |
| src/net/ds_https_request.c (死) | esp_tls 裸 HTTPS 循环（心知天气+B站），响应仅打印 | ds_https_request_init:190 | 无 |
| src/ui/ds_ui_page_manage.c | 页面状态机 + UI 事件队列分发 | send_event:28、send_action:57、ds_ui_get_now_show_page:128、init:132 | 各页面模块、ds_screen、ds_http_request、ds_pwm |
| src/ui/ds_ui_mainpage.c | 主页/设置页整图全刷 | ds_ui_back_main_page:17(死)、ds_screen_display_main:24、ds_screen_setting:31 | ds_screen、ds_spi、ds_data_page、ds_wifi_ap_sta |
| src/ui/ds_ui_timepage.c | 时钟页（合成全刷 + 分钟级局刷） | show_time_init:58、show_time:143、updatetime:172、init:188 | ds_screen、ds_spi、ds_system_data、ds_data_num/icon |
| src/ui/ds_ui_weatherpage.c | 天气页（Paint 画布+字库+图标） | ds_ui_weather_set:43、show:59、init:147 | ds_paint(→ds_font)、ds_screen、ds_data_* |
| src/ui/ds_ui_wordpage.c | 单词页（4 组内置中英文循环） | show:50、init:98 | ds_paint、ds_screen、ds_data_* |
| src/ui/ds_ui_tomatopage.c | 番茄钟页（倒计时+启停+图标） | show_init:119、start_toggle:172、updatetime:187、time_set:235、init:244 | ds_screen、ds_spi、ds_system_data、ds_data_num/icon、ds_nvs、ds_pwm |
| src/ui/ds_ui_fans.c | B站粉丝页（两屏轮换；**不可达页面**） | int_to_char:32、show_init:52、set_fans_num:97、set_aqifans_num:101、init:105 | ds_paint、ds_screen、ds_data_* |
| src/ui/ds_ui_num_display.c | **未编译**：不在 CMake SRCS，全文皆注释 | — | — |
| src/draw/ds_font.c | 从 storage1 分区读 XBF 字库 | ds_get_bitmap:93、test_ds_font:110(死) | 无（仅 esp_partition） |
| src/draw/ds_paint.c | 绘图原语 + 画布 + SPI 写屏 | UTF8toUnicode:9、Paint_NewImage:79、SelectImage:111、SetPixel:124、Clear:197、ClearWindows:216、DrawPoint:235、DrawLine:275、DrawRectangle:333、DrawCircle:366、DrawString_CN:436、DrawString_piture:492、ds_paint_image:511、ds_paint_last_page:521、ds_paint_image_new:528、ds_paint_image_copy:536 | ds_screen、ds_spi、ds_font |
| src/ds_system_data.c | 全局状态单例的全部访问函数（32 个） | ds_system_data_init:16 … update_system_time_minute:290（:16-298，字段见 §9） | ds_nvs、ds_ui_page_manage、ds_ui_tomatopage（＋include ds_ui_weatherpage） |
| src/ds_test.c (死) | 仅 printf("ds_test") | ds_test:5 | 无 |

---

## 2. 硬件资源表

### GPIO（全部定义于 ds_gpio.c / ds_spi.c，已逐一核实）

| GPIO | 用途 | 定义/配置 file:line |
|---|---|---|
| 4 | FT6336 触摸 INT：输入、上拉、ANYEDGE 中断、挂 ISR；兼作浅睡唤醒脚（低电平） | ds_gpio.c:22、85-92、106；唤醒 app_main.c:61、81 |
| 5 | FT6336 触摸 RST：输出，0=复位 | ds_gpio.c:20、181-183；复位时序 ds_ft6336.c:144-147 |
| 12 | 屏幕 RST：输出，0=复位 | ds_gpio.c:33-34 |
| 13 | 屏幕 BUSY：输入、上拉、NEGEDGE 中断类型（**未挂 ISR，仅轮询**，挂接代码被注释 ds_gpio.c:155-158）；1=忙 | ds_gpio.c:36、144-151 |
| 14 | 屏幕 D/C：输出，1=data 0=cmd | ds_gpio.c:30 |
| 15 | 蜂鸣器 PWM 输出（MCPWM1A） | ds_pwm.c:19、80 |
| 23 | SPI MISO（已配置，屏幕只写不读） | ds_spi.c:28 |
| 25 | SPI CLK | ds_spi.c:30 |
| 26 | SPI MOSI | ds_spi.c:29 |
| 27 | 屏幕 CS：输出，0=有效，**软件控制**（与 SPI 设备无关） | ds_gpio.c:27；ds_spi.c:31 |
| 32 | I2C SCL（内部上拉） | ds_i2c.h:9 ← sdkconfig CONFIG_I2C_MASTER_SCL=32 |
| 33 | I2C SDA（内部上拉） | ds_i2c.h:10 ← CONFIG_I2C_MASTER_SDA=33 |

无背光、无电源使能、无独立按键 GPIO（“按键”即 GPIO4 触摸 INT 双用途）。被注释的旧引脚组：MISO 0/MOSI 5（ds_spi.c:23-26）。

### SPI
- Host：`HSPI_HOST`（=SPI2），ds_spi.c:98、101。
- 总线：MISO=23/MOSI=26/SCLK=25，`max_transfer_sz=512`（64*8），**dma_chan=0（不用 DMA）**（ds_spi.c:82-98；`#define DMA_CHAN 2` 未使用，ds_spi.c:21）。
- 设备：**15 MHz**（`15*1000*1000`，ds_spi.c:92；注释写 26MHz 与实际不符）、**mode 0**（CPOL=0/CPHA=0，ds_spi.c:93）、queue_size=7（:94）、**无硬件 CS**（软件 GPIO27）、`.pre_cb` 被注释（:95）。
- 传输方式：**轮询** `spi_device_polling_transmit`（ds_spi.c:50、65），每次事务固定 8 bit 单字节（:47、62）；每字节事务前后手工拉 CS/DC（:43-44、51、59-60、66）。

### I2C
- 端口 **I2C_NUM_1**（ds_i2c.h:11；CONFIG_I2C_MASTER_PORT_NUM=1，sdkconfig），频率 **100 kHz**（CONFIG_I2C_MASTER_FREQUENCY；ds_i2c.c:92），RX/TX 缓冲禁用（ds_i2c.c:94）。
- FT6336 地址 **0x38（7 位）**（ds_i2c.h:16），线上按 `(0x38<<1)|R/W` 发送（ds_i2c.c:24、45、67）。
- **旧版命令链 API**（i2c_param_config/i2c_driver_install/i2c_cmd_link_*，ds_i2c.c:22-94），超时 1000 tick（:27、53、72）。

### PWM —— 是 MCPWM，不是 LEDC
- MCPWM_UNIT_1 / TIMER_1 / GEN_A → MCPWM1A，GPIO15（ds_pwm.c:80）；频率 **4000 Hz**（:82，注释写 50Hz servo 与实际不符）、向上计数（:84）、DUTY_MODE_0（:85）。
- 响=50% 占空（:23），停=强制低电平（:31）。用途：蜂鸣器（有源/无源未标注）。**全仓库无 ledc_* 调用**，无“分辨率位数”概念（MCPWM 计数器精度由时钟/分频决定）。

### 定时器
- **旧版 hw timer 驱动**（driver/timer.h）：TIMER_GROUP_0/TIMER_0，divider=16，向上计数、自动重载，alarm = 10×TIMER_SCALE（TIMER_BASE_CLK/16/1000）→ **10ms 周期**（ds_timer.c:23-26、89-108）。ISR 以 ESP_INTR_FLAG_IRAM 注册（:105-106）。**无 esp_timer_create、无 gptimer、无 FreeRTOS 软件定时器**（esp_timer 仅用 `esp_timer_get_time()`，app_main.c:89、96）。
- 浅睡唤醒源：60s 定时器 + GPIO4 低电平（app_main.c:81-84）。

---

## 3. ESP-IDF / FreeRTOS API 使用清单

（行号来自 grep 原始输出；"(死)"=所在代码路径当前不执行）

| 类别 | 函数 | 位置 |
|---|---|---|
| gpio | gpio_config（输出/输入/上拉/ANYEDGE/NEGEDGE） | ds_gpio.c:82,92,132,137,142,151 |
| | gpio_set_level（CS/DC/RST/TP-RST） | ds_gpio.c:166,170,174,182 |
| | gpio_get_level（BUSY、TP INT 电平） | ds_gpio.c:53,56,178 |
| | gpio_install_isr_service / gpio_isr_handler_add / remove | ds_gpio.c:104,106,112,116 |
| | gpio_wakeup_enable / gpio_wakeup_disable | app_main.c:81,108,116 |
| spi_master | spi_bus_initialize / spi_bus_add_device | ds_spi.c:98,101 |
| | spi_device_polling_transmit | ds_spi.c:50,65 |
| i2c（旧驱动） | i2c_param_config / i2c_driver_install | ds_i2c.c:93,94 |
| | i2c_cmd_link_create/delete、i2c_master_start/stop、i2c_master_write_byte、i2c_master_read_byte、i2c_master_write、i2c_master_cmd_begin | ds_i2c.c:22-28,43-54,64-73 |
| mcpwm | mcpwm_gpio_init / mcpwm_init / mcpwm_set_duty / mcpwm_set_duty_type / mcpwm_set_signal_low | ds_pwm.c:80,87,23,25,31 |
| timer（旧 hw 驱动） | timer_init / timer_set_counter_value / timer_set_alarm_value / timer_enable_intr / timer_isr_register / timer_start | ds_timer.c:96,100,103,104,105,108 |
| | timer_group_clr_intr_status_in_isr / timer_group_enable_alarm_in_isr | ds_timer.c:60,64 |
| esp_timer | esp_timer_get_time（仅计时测量） | app_main.c:89,96 |
| esp_sleep | esp_sleep_enable_timer_wakeup / esp_sleep_enable_gpio_wakeup / esp_light_sleep_start / esp_sleep_get_wakeup_cause / esp_sleep_disable_wakeup_source | app_main.c:83,84,92,100,109,117 |
| esp_task_wdt | esp_task_wdt_reset / delete（init/add 被注释 app_main.c:199-200） | app_main.c:71,253,260 |
| nvs | nvs_flash_init | app_main.c:203 |
| | nvs_open / nvs_get_u8 / nvs_get_str / nvs_set_u8 / nvs_set_str / nvs_commit / nvs_close | ds_nvs.c:31-155（各处，详见 §7 key 表） |
| esp_partition | esp_partition_find_first(0x40,0x01,NULL) / esp_partition_read | ds_font.c:47,59 |
| spi_flash（旧） | spi_flash_get_chip_size | app_main.c:193 |
| spiffs | esp_vfs_spiffs_register / esp_spiffs_info / esp_spiffs_format / esp_spiffs_check | ds_spiffs.c:35,48,51,60 |
| esp_wifi | esp_wifi_init / set_mode(APSTA) / set_config(STA,AP) / start / stop / deinit / connect | ds_wifi_ap_sta.c:74,77,106,107,108,137,138,31,33 |
| | esp_wifi_scan_start / get_ap_records / get_ap_num / scan_stop / deinit（死） | ds_wifi_scan.c:116-139 |
| esp_netif | esp_netif_init / create_default_wifi_sta / create_default_wifi_ap / destroy_default_wifi(sta/ap) | app_main.c:204；ds_wifi_ap_sta.c:70,71,135,136 |
| esp_event | esp_event_loop_create_default / handler_register / handler_unregister | app_main.c:205；ds_wifi_ap_sta.c:75,76,133,134（死注册：ds_wifi_sta.c:86,87、ds_wifi_ap.c:60、ds_wifi_scan.c:102） |
| esp_http_client | esp_http_client_init / perform / cleanup（×4 组） | ds_http_request.c:466-467,475；504-505,513；538-539,547；559-560,568 |
| esp_http_server | httpd_start / httpd_register_uri_handler（×6） | ds_http_server.c:590,602,611,620,629,638,647 |
| esp_tls | esp_tls_conn_http_new / conn_write / conn_read / conn_delete（**全部死路径**） | ds_https_request.c:96,98,114,128,149,175 |
| esp_crt_bundle | **未使用**（grep 无） | — |
| lwip socket | 仅 ds_dns_server.c 注释代码中（socket/recvfrom/sendto/bind，死） | ds_dns_server.c:27,63,72,134,286 |
| sntp | sntp_setoperatingmode / sntp_setservername / sntp_set_time_sync_notification_cb / sntp_init / sntp_stop | ds_sntp.c:61,62,63,64,68-69 |
| cJSON | cJSON_Parse / cJSON_GetObjectItem | ds_http_request.c:56-216；ds_http_server.c:436,438,439,477,479-481,517,519 |
| FreeRTOS 任务 | xTaskCreate ×8（存活） | app_main.c:244；ds_ui_page_manage.c:135；ds_timer.c:167,168；ds_pwm.c:92；ds_gpio.c:101；ds_http_request.c:645；ds_wifi_ap_sta.c:179（死：ds_https_request.c:193）。无 xTaskCreatePinnedToCore |
| FreeRTOS 队列 | xQueueCreate ×7 / xQueueReceive / xQueueSend / xQueueSendFromISR | 见 §4 队列表 |
| FreeRTOS 事件组 | xEventGroupCreate / SetBits / WaitBits（**死路径**，ds_wifi_sta 模块不启动） | ds_wifi_sta.c:70,76,126-130 |
| FreeRTOS 其它 | vTaskDelay、portMAX_DELAY（广泛）；**无 xSemaphore*、无 xTimerCreate** | — |
| libc 时间 | setenv("TZ","CST-8") / tzset / time / localtime_r | ds_sntp.c:33,34,51,52 |

---

## 4. 并发模型

### 任务（全部 xTaskCreate，无绑核；无任何互斥锁/信号量/软件定时器）

| 任务名 | 创建处 | 栈 | 优先级 | 入口 | 循环行为 |
|---|---|---|---|---|---|
| background_task | app_main.c:244 | 4096 | 10 | app_main.c:130 | 每 1s：首联网触发 HTTP+SNTP（137-141）；下拉刷新（143-153）；AP&STA 10 分钟自动关（156-167，初值 590）；设置页自动开 AP&STA（169-174） |
| ui_page_evt_task | ds_ui_page_manage.c:135 | 4096 | 5 | :63 | 阻塞收 ui_event_queue（:67），if/else 链切页并直接调页面刷新函数（:70-124） |
| timer_evt_task | ds_timer.c:167 | 2048 | 5 | :114 | 阻塞收 timer_queue（:118）→ 每 10ms：count_tp_action_manage_time（:119）；status 1-2 采触摸起点（:121-124）、>2 采终点（:125-128） |
| ui_timer_update_task | ds_timer.c:168 | 2048 | 5 | :136 | 每 10ms 计数：满 6000（60s）→ ds_ui_timepage_updatetime（:143-146）；满 100（1s）→ ds_ui_tomatopage_updatetime（:147-152） |
| beep_task | ds_pwm.c:92 | 1024 | 10 | :34 | 阻塞收 beep_queue（:38），按 BEEP_SHORT_100MS/500MS/LONG 用 50% duty 100ms 组合鸣叫（:39-63） |
| gpio_task_example | ds_gpio.c:101 | 2048 | 10 | :48 | 阻塞收 gpio_evt_queue（:52）：TP 中断 → set_tp_wackup_timeleft(60)（:55）；电平 0（按下）reset_tp_action_manage（:58）、电平 1（抬起）check_tp_action（:61） |
| http_request_task | ds_http_request.c:645 | 4096 | 5 | :614 | 阻塞收 http_request_event_queue（:618）→ 延时 1s → switch 分发 → 延时 2s（:620-638）。**现存唯一事件源只发 HTTP_GET_WEATHER**（:611），TIME/FANS/CITY 分支不可达 |
| wifi_net_task | ds_wifi_ap_sta.c:179 | 4096 | 10 | :141 | 阻塞收 wifi_event_queue（:148）：START→ds_wifi_ap_sta_start（:152-156）；UPDATE→仅重写 STA 配置（:158-160→:111-129）；STOP→注销事件+销毁 netif+stop/deinit（:161-164→:131-139） |
| https_get_task（死） | ds_https_request.c:193 | 8192 | 5 | :80 | 因 ds_https_request_init 无调用者，任务从不创建 |

### 队列

| 队列 | 创建处 | 元素×深度 | 生产者 → 消费者 |
|---|---|---|---|
| ui_event_queue | ds_ui_page_manage.c:134 | UI_EVENT_T{key,touch_x,touch_y,action}（:17-23）×10 | send_event:53 / send_action:60 → ui_page_evt_task:67 |
| timer_queue | ds_timer.c:164 | timer_event_t ×10 | 定时器 ISR:73 → timer_evt_task:118 |
| ui_update_timer_queue | ds_timer.c:165 | timer_event_t ×10 | 定时器 ISR:74 → ui_timer_update_task:140 |
| beep_queue | ds_pwm.c:90 | uint32_t ×10 | send_beep_event:70 / _from_isr:75 → beep_task:38 |
| gpio_evt_queue | ds_gpio.c:99 | uint32_t ×10 | gpio_isr_handler:45 → gpio_task_example:52 |
| http_request_event_queue | ds_http_request.c:644 | 枚举 ×10 | ds_http_request_type:607 → http_request_task:618 |
| wifi_event_queue | ds_wifi_ap_sta.c:178 | 枚举 ×10 | ds_wifi_send_event:174 → wifi_net_task:148 |

### ISR / 事件处理器
- `gpio_isr_handler`（IRAM_ATTR，ds_gpio.c:42-46）：仅把 GPIO4 编号入队。
- `timer_group0_isr`（IRAM_ATTR，ds_timer.c:51-76）：清中断、重开 alarm（:60-64）；`timer_second_count_isr` 每 100 次（1s）**直接在 ISR 内调用 `update_system_time_second()`**（:69-72，该函数非 IRAM_ATTR，位于 ds_system_data.c:274）；随后向两个队列各发一个**未初始化的** `evt` 结构（:58、73-74，字段从未赋值）。
- esp_event `wifi_event_handler`（ds_wifi_ap_sta.c:24-66，注册于 :75-76）：STA_START→connect（:30-31）；STA_DISCONNECTED→**无条件立即重连**（:32-35，重试上限逻辑被注释 ：36-45）；**IP_EVENT_STA_GOT_IP→蜂鸣 + 切回主页 + 置 CONNECT_SUCCESS**（:46-53）；AP 连接/断开→蜂鸣+状态（:56-65）。
- esp_http_client 事件回调（任务上下文）：_http_time/weather/fans/city_event_handle（ds_http_request.c:224,282,356,422），ON_DATA 时调对应 cjson_* 解析。
- SNTP 回调 time_sync_notification_cb（ds_sntp.c:47-54）。
- httpd 内部任务（httpd_start，ds_http_server.c:590）。

### 并发相关事实（移植时需知的行为现状）
- `ds_ui_page_manage_send_event/send_action` 统一用 `xQueueSendFromISR`（ds_ui_page_manage.c:53,60），但其调用方多在**任务上下文**（app_main.c:235/238、ds_wifi_ap_sta.c:51、ds_http_server.c:552、ds_system_data.c:188 等）。
- `send_event` 只在 SHORT 分支给 `evt.action` 赋值（ds_ui_page_manage.c:33-51）；其它手势下 action 为未初始化栈值入队。`send_action` 则 key/x/y 未初始化（:57-61）。
- `get_system_data()` 按值返回整个 SYSTRM_DATA_T（ds_system_data.c:25-27），多处整结构拷贝读取。
- 全局状态（g_system_data、g_page_manage、各 g_*_page、partial_data）无锁共享。

---

## 5. 主流程

### app_main 初始化顺序（app_main.c:178-261，已逐行核实）

| 行 | 调用 | 说明 |
|---|---|---|
| 202 | ds_system_data_init() | 清零全局状态，置 first_time_httpdata_request=true、hour=10、minute=10 |
| 203-205 | nvs_flash_init / esp_netif_init / esp_event_loop_create_default | |
| 208 | ds_http_request_init() | 建队列+http_request_task |
| 209 | ds_http_server_init() | init_spiffs + httpd 常驻（**开机即起，不随 AP 关闭**） |
| 211 | ds_wifi_ap_sta_init() | 建队列+wifi_net_task（此刻只建任务，未启动 WiFi） |
| 214 | init_ft6336() | TP 复位+I2C 初始化+寄存器配置；内部先 ds_touch_gpio_init()（建 gpio 队列/任务/ISR） |
| 215 | init_screen_interface() | 屏幕 GPIO+SPI（ds_screen.c:316-319） |
| 217-223 | 各页面 init | timepage→page_manage→weather→wordpage（**:220-221 连续调用两次**）→tomato→fans |
| 225 | ds_timer_init() | 10ms 硬件定时器 + 2 任务 |
| 227 | ds_pwm_init() | MCPWM 蜂鸣器 + beep 任务 |
| 233-239 | ds_nvs_read_wifi_info() | 有存 WiFi→send_action(MEMU)；无→send_action(SETTING) |
| 242-243 | 延时 2s → ds_wifi_send_event(AP_STA_START) | 启动 AP+STA |
| 244 | xTaskCreate(background_task) | |
| 247 | sleep_mode_init() | **阻塞进入 while(true) 浅睡管理循环，此后代码不可达**（:249-260 的 for 循环与 esp_task_wdt_delete 为不可达代码） |

`sleep_mode_init`（app_main.c:65-128）：条件=「AP&STA 已关 && 在主页 && 600s 无触摸」（:75，SLEEP_TIME_RESET=600 :63）→ 摘 TP ISR、使能 GPIO4 低电平+60s 定时器唤醒 → `esp_light_sleep_start`（:92）→ TIMER 唤醒：仅 `update_system_time_minute()`（:104）+timeleft=1（很快再睡）；GPIO/其它唤醒：重挂 TP ISR+reset_tp_action_manage（:108-120），timeleft=60。

### 页面管理
- 页面枚举 PAGE_TYPE_E：MEMU(0)/TIME/WEATHER/WORD/TOMATO/FANS/SETTING（ds_ui_page_manage.h:13-21）；当前页存全局 `g_page_manage.now_show_page`（ds_ui_page_manage.c:15）。**不是函数指针表**，是 `ui_page_evt_task` 的 if/else 链（:70-124）。
- 主页短按按坐标 (x<75?, y<75?) 四象限 → TIME/WORD/WEATHER/TOMATO（:33-51）；主页长按 → SETTING（:93-95）；主页左滑进 FANS 的分支被注释（:97-100）→ **FANS 页不可达**；主页下拉(MOVE_DOWN) → set_update_httpdata_request(true)（:101-103）；非主页右滑 → 回主页全刷（:120-123）；TOMATO 页短按 → 启停（:116-118）。
- 触摸事件流：FT6336 INT(GPIO4 双沿) → gpio_isr_handler（ds_gpio.c:42）→ gpio_evt_queue → gpio_task_example（按下 reset/抬起 check_tp_action，ds_gpio.c:54-62）；并行地 10ms 定时器任务轮询 I2C 采起点/终点（ds_timer.c:121-128）；抬起时 `check_tp_action()`（ds_system_data.c:177-238）按位移/时长判 SHORT/LONG/LEFT/RIGHT/UP/DOWN 并 `ds_ui_page_manage_send_event`（:188/201/214/225）→ 蜂鸣+入队（ds_ui_page_manage.c:52-53）→ ui_page_evt_task 分发。**坐标读取靠定时器轮询 I2C，不靠中断读数**。

### 刷新策略
- 全刷入口：`ds_screen_full_display_bydata`（主页/设置页，ds_ui_mainpage.c:26/28/32）；`ds_screen_full_display`（Paint 类页面：weather/word/fans；合成页 `ds_screen_partial_data_copy` 内部，ds_screen.c:312）。
- 局刷：`ds_screen_partial_display_bydata`（**不自带刷新**）+ 调用方 `refresh_part()`+`deep_sleep()`（时钟页 ds_ui_timepage.c:154-165；番茄页 ds_ui_tomatopage.c:102-113）。
- 节流：时钟页 `partial_update_time>5` → 改走合成全刷（ds_ui_timepage.c:177-183）；番茄页同逻辑（ds_ui_tomatopage.c:219-228），工作/休息轮换时强制全刷（:210-214）。天气/单词/粉丝页无计数，固定全刷。
- 触发时机：① 定时器派生（60s 时钟页、1s 番茄秒计数）；② 触摸切页；③ 网络（GOT_IP → 切回主页全刷；POST /back → 主页全刷）；天气数据到达**不直接刷屏**，只写 g_weather_page，下次进页才刷；④ 浅睡 TIMER 唤醒只改数据不刷屏。

---

## 6. 字库与图片

### 字库 ds_font.c（已全文核实）
- 读取 API：`esp_partition_find_first(0x40, 0x01, NULL)`（ds_font.c:47，即 storage1；首次调用后缓存 partition_res，:37、43-58）→ **`esp_partition_read(partition_res, offset, __g_font_buf, size)`**（:59）读入静态缓冲 `__g_font_buf[300]`（:35）。未用 spi_flash_mmap。
- 文件头在固件中**硬编码**（不读 bin 头）：`x_header_t{u16 min=0x0020, u16 max=0x9FA0, u8 bpp=4, u8 rsv[3]}`（:7-12、27-31）。
- 偏移计算（Unicode，**不是 GB2312**；范围 U+0020–U+9FA0 覆盖 ASCII+常用汉字）：
  - 索引表项地址 = `sizeof(x_header_t) + (unicode - 0x20) * 4` = `8 + (unicode-0x20)*4`（:71）
  - 该 u32 为字形绝对偏移 pos（:72-74）；pos 处 7 字节 `glyph_dsc_t{adv_w,box_w,box_h,ofs_x,ofs_y,r}`（:14-21、75-76）
  - 点阵位于 `pos + 7`（:90），字节数 `size = box_w*box_h*4/8`（:97，**4bpp**）
- 单一字库、无字号分档（每字形自带变宽 box_w/box_h）；注释标注 “SimHei 字模高度 18 XBF 外部 bin”（:121-123）。
- 显示链路：`Paint_DrawString_CN`（ds_paint.c:436-490）逐字符 `UTF8toUnicode`（ds_paint.c:9-69）→ `ds_get_bitmap`（ds_font.c:93-107，memcpy 到调用方 300 字节缓冲）→ 按 4bit 半字节消费：`(tmp>>index)&0x0f >= 1` 画背景色否则前景色（二值化使用，:468-473）→ 字节步进：首字节 ≥128 时 `i+=3`（**中文 3 字节，标准 UTF-8**；:479-483 注释写"4byte"与代码不符）。

### 图片/数据（main/src/data/，声明已 grep 核实；统一 **1bpp、按行打包、字节内 MSB first**，字节数=ceil(W/8)×H）
| 文件 | 数组 | 尺寸/内容 |
|---|---|---|
| ds_data_num.c:4 | gImage_num_size48[10][192] | 数字 0-9，32×48（命名 48 与注释混用） |
| ds_data_num.c:159 | gImage_num_size24[10][48] | 数字 0-9，16×24 |
| ds_data_num.c:330 | gImage_time_symbol[36] | 8×36 时间冒号 |
| ds_data_num.c:235-326 | gImage_num_size24[10][72] | 24×24 旧版数字（整段注释，死） |
| ds_data_weather.c:2 | gImage_num_weather_icon[7][288] | 7 张 48×48 天气图标（晴/多云/阴/大风/暴雨/小雨/雪） |
| ds_data_weather.c:153 | gImage_num_weather_code[11][128] | 11 个 32×32 汉字点阵（晴多云阴大小雨暴夹雪） |
| ds_data_weather.c:278 | gImage_city_tianqi[2][128] | 2 个 32×32 汉字 |
| ds_data_icon.c:5-179 | word/weather/TH/rest/work/click_start/tomato_finish | 7 个 56×56 图标（392B） |
| ds_data_icon.c:208-296 | back(**非 const**,唯一)/add/dec/set/template/humidity；fans | 36×36 ×6（180B）+ 32×32 fans（128B）；长度全局 icon_*_len（:308-310） |
| ds_data_page.c:4/321/638 | gImage_main_page_wifi / gImage_main_page / gImage_setting_page | 3 张 **200×200 整页背景（各 5000B）**，952 行中 942 行是十六进制数据 |
| include/data/ds_data_image.h:5-44 | lut_vcomDC/ww/bw/wb/bb | 全屏刷 LUT 数组定义在头文件（局刷 LUT :50-83 被注释）；**注意 ds_screen.c 实际未装载任何 LUT 命令** |

另：ds_screen.c 内还自带 gImage_logo[5000]（:493）与 gImage_num1/2/3[192]（:809-853，demo 用，死）。

---

## 7. 网络功能

### 配网（captive portal 实际上**不存在**）
- 模式：AP+STA 同时（`esp_wifi_set_mode(WIFI_MODE_APSTA)`，ds_wifi_ap_sta.c:77）。AP SSID=`"桌面屏幕"`（sdkconfig CONFIG_ESP_AP_WIFI_SSID）、密码 `"12345678"` 但 **authmode=WIFI_AUTH_OPEN（:103，开放网络）**、channel 硬编码 11（:100）、max_conn=4；IP 为 netif 默认 **192.168.4.1**（`esp_netif_create_default_wifi_ap`，:71）。
- **DNS 劫持：无**。ds_dns_server.c 全部实现被注释（53/UDP 劫持逻辑 ：21-143 为 lwip socket 注释代码，回填 192.168.4.1），入口 `dns_server_start`:145 与 `web_server_start`:330 均为空函数（其 xTaskCreate 也被注释）。启动引用也仅在 ds_wifi_ap_sta.c:144-145 被注释。
- HTTP 服务器（**开机常驻**，端口 80，`httpd_uri_match_wildcard`，ds_http_server.c:587）：
  | 路由 | 方法 | handler | 行为 |
  |---|---|---|---|
  | `/*` :596 | GET | download_get_handler:221 | `/`→输出**内嵌 setting.html**（:112-117）+SPIFFS 文件列表；`/index.html`→307→`/`；`/favicon.ico`→内嵌 ico；SPIFFS 文件按 8192B 下发 |
  | `/upload/*` :605 | POST | upload_post_handler:299 | 写文件到 /spiffs，≤200KB，成功 303→`/` |
  | `/wifi_data` :614 | POST | send_wifi_handler:409 | cJSON 取 wifi_name/wifi_code → **ds_nvs_save_wifi_info**:440 → ds_wifi_send_event(AP_STA_UPDATE):442 + 蜂鸣 |
  | `/tomato_data` :623 | POST | send_tomato_handler:450 | work_time/rest_time/time_count → set_system_data_tomato_info:482（→ ds_ui_tomatopage_time_set → NVS 保存，TO:236） |
  | `/city_data` :632 | POST | send_city_handler:490 | city → set_system_data_city_info:520（→ NVS save_city，SD:113） |
  | `/back` :641 | POST | send_back_handler:528 | 蜂鸣 + send_action(PAGE_TYPE_MEMU):552 |
- setting.html 表单：wifi 账号/密码（#wifi/#code，:160-161）→POST /wifi_data JSON `{wifi_name,wifi_code}`（:220-247）；番茄 work/rest/count 下拉（默认 20/5/3）→POST /tomato_data（:249-276）；城市 guangzhou/shenzhen/shanghai/beijing（默认 beijing）→POST /city_data（:278-301）；退出→POST /back（:303-325）。（setting.html 行号来自子代理，handler 侧已亲核）
- STA 连接：WIFI_EVENT_STA_START→esp_wifi_connect（:30-31）；断开→**无条件重连**（:32-35）；拿到 IP→蜂鸣+回主页+置成功（:46-53）。STA 默认 ssid/psw 来自 sdkconfig（"leo"/"123456789"，CONFIG_ESP_WIFI_SSID/PASSWORD），有 NVS 存储时覆盖（:91-94）；threshold WPA2、pmf capable（:83-87）。
- WiFi 扫描结果**不进**网页（ds_wifi_scan.c 仅打印，死模块；setting.html 无扫描列表）。

### 出站请求与解析（ds_http_request.c；均 GET + `skip_cert_common_name_check=true`，HTTP 非 HTTPS）
| URL（逐字） | 触发 | 解析（cJSON） | 去向 |
|---|---|---|---|
| `http://quan.suning.com/getSysTime.do`（:462，**废弃**，注释注明改用 sntp :455） | HTTP_GET_TIME（不可达） | `sysTime1` 下标 8-13 拼 时/分/秒（:59-75） | update_system_time |
| `http://api.seniverse.com/v3/weather/now.json?key=SmazqPcltzTft-X3v&location=<NVS城市，缺省 guangzhou>&language=zh-Hans&unit=c`（:483-494） | HTTP_GET_WEATHER（`ds_http_request_all`:610-612 唯一存活事件） | `results[0].location.name`→city、`results[0].now.text`、`now.code`、`now.temperature`（:117-144） | 全局 `weathe`(:79-87) + ds_ui_weather_set → g_weather_page |
| `http://api.bilibili.com/x/relation/stat?vmid=383943678&jsonp=jsonp`（小智）/:527 `vmid=257459324`（阿奇）（:524/527，**死**，注释“B站需https” :517） | HTTP_GET_FANS（不可达） | 解析调用被注释（:365-380）；cjson_fans_info:165-181 取 `data.follower` 但无存活调用 | — |
| `http://pv.sohu.com/cityjson?ie=utf-8`（:554） | HTTP_GET_CITY（不可达） | `cname`/`cid` 只存局部变量，未使用（:204-218） | — |

HTTPS（ds_https_request.c，**整个模块死**）：`https://api.seniverse.com/...同上`（:47）、`https://api.bilibili.com/x/relation/stat?vmid=383943678&jsonp=jsonp`（:50）；证书为**嵌入的单 CA pem**（server_root_cert.pem=DigiCert 系、server_root_cert_bilibili.pem=GlobalSign 系）经 esp_tls_cfg_t.cacert_buf（:74-93），**未用 esp_crt_bundle**；esp_tls 手写 HTTP/1.0 请求（:54-62），响应仅 putchar 打印、无任何字段落地（:149-172）。

### SNTP（ds_sntp.c）
- 服务器 `pool.ntp.org`（:62，仅一个），POLL 模式（:61），回调注册（:63）；首次完整 init，再次 sntp_stop+sntp_init（:66-70）。
- 时区：回调内 `setenv("TZ","CST-8",1); tzset();`（:51-52）→ `time()`+`localtime_r`（:33-34）→ `update_system_time(h,m,s)`（:44）写 g_system_data.hour/minute/second。
- 调用时机：首次联网成功 + 每次下拉刷新（app_main.c:137-141、143-149）。

### NVS key 全表（grep 核实）
| namespace | key | 类型 | 写 | 读 |
|---|---|---|---|---|
| wificonfig | wifi_flag | u8（1=已存） | ds_nvs.c:37,86 | :53 |
| wificonfig | ssid / psw | str | :38,39 | :66,67 |
| cityconfig | city | str | :99 | :113,115 |
| tomatoconfig | worktime / resettime / roundtimes | u8 | :133-135 | :151-153（默认 20/5/3，:143-145） |

无 nvs_erase_key；“清除” = 写 flag=0（:77-89）。SPIFFS 无固定文件，仅文件服务器通配读写（/spiffs/<name>，ds_http_server.c:221-407；挂载参数 ds_spiffs.c:28-35：base_path=/spiffs、label=NULL→storage 分区、max_files=5、format_if_mount_failed）。

---

## 8. 屏幕驱动细节（ds_screen.c 1-330 已亲核）

- **分辨率/帧缓冲**：200×200，**1bpp**，整屏 25 字节/行 × 200 = **5000 字节**（IMAGE_SIZE，ds_ui_page_manage.h:8-10）。屏内无 RAM 缓冲常驻——每次刷新函数直接把 5000 字节逐字节经 SPI 写入 0x24 RAM。两块合成缓冲：`partial_data[200][25]`（:258，页内拼图用）与 `partial_data_array[5000]`（:259，展平后全刷）。Paint 画布同为 200×200/5000B（ds_paint.c，PAINT 结构 WidthByte=25）。
- **初始化序列**（`init_display` :37-81，全刷前每次执行）：
  1. RST 低 10ms→高→100ms（:39-42）；等 BUSY（:44）
  2. `0x12` SWRESET（:45）→ 等忙
  3. `0x01` Driver Output Control = `C7 00 01`（199+1 行，:48-51）
  4. `0x11` Data Entry Mode = `01`（:53-54）
  5. `0x44` RAM X 窗口 = `00 18`（(24+1)*8=200，:56-58）
  6. `0x45` RAM Y 窗口 = `C7 00 00 00`（0..199，:60-64）
  7. `0x3C` BorderWaveform = `05`（:66-67）
  8. `0x18` 温度传感器 = `80`（:69-70）
  9. `0x4E`/`0x4F` RAM X/Y 计数器 = `00` / `C7 00`（:72-76）
  10. 100ms+等忙（:79-80）。**不装载 LUT（无 0x32/0x23）、不写 VCOM（无 0x2C）**（ds_data_image.h 里的 LUT 数组未被驱动使用）。
- **全刷**：`ds_screen_full_display[_bydata]`（:135/:143）= init_display → 回调写 `0x24`（5000 字节）→ `refresh()` → `deep_sleep()`。`refresh()`（:91-97）= `0x22` 参数 `0xF7` + `0x20` + 等忙。`ds_screen_display_data`（:291-301）同时写 `0x24` 与 `0x26` 各 5000 字节。
- **局刷**：`ds_screen_partial_display_bydata`（:204-256）：x/8、x_end=x_start+宽/8-1；**Y 翻转** `y_start2=200-y_start`（≥256 拆高低字节，:215-227）；RST 复位脉冲 10ms（防底色变化，:229-232）；`0x3C=0x80` 锁边框（:234-235）；`0x44`/`0x45` 设窗口、`0x4E`/`0x4F` 设计数器（:237-250）；`0x24` 写窗口数据（:252-253）——**不发 0x20 旧数据、不用 0x90/0x92 partial window 命令**；函数结束**不刷新不睡眠**，由调用方 `refresh_part()`（:99-105 = `0x22`+`0xFF`+`0x20`+等忙）+ `deep_sleep()`。合成路径 `partial_data_add`（:269-288）把 1bpp 图片**按字节取反 `~data`** 写入 partial_data。
- **BUSY 等待**：`lcd_chkstatus`（:17-35）轮询 GPIO13，`&0x01`，**1=忙**；每 10ms 一次，count≥1000 打印 timeout 退出（上限 10s）。demo 用的 `Epaper_READBUSY`（:332-338）为无延时无超时死循环（死代码）。
- **睡眠**：每次刷新后发 `0x10 0x01` deep sleep + 100ms（:84-89）。
- 其它事实：`ds_screen_display_white`（:109-119）写 250×25=**6250 字节**（超出 5000 字节窗口）；`EPD_HW_Init`（:365-407）为 init_display 的重复实现（仅 demo 链用）；无软件旋转/镜像（仅局刷 Y 地址翻转与数据取反）；触摸坐标另有 200→152 缩放（ds_ft6336.c:78-79）。

---

## 9. ds_system_data 全局状态

单例 `SYSTRM_DATA_T g_system_data`（ds_system_data.c:14），结构定义 ds_system_data.h:59-100（已亲核）：

| 字段 | 类型 | 行 | 含义/备注 |
|---|---|---|---|
| setting_ssid / setting_ssid_len | char[32] / u8 | h:62-63 | 配网 SSID |
| setting_psw / setting_psw_len | char[64] / u8 | h:64-65 | 配网密码 |
| has_wifi_info | bool | h:66 | 是否已存 WiFi |
| wifi_ap_status / wifi_sta_status | 枚举 | h:69-70 | AP(INIT/CONNECT/DISCONNECT)、STA(INIT/SUCCESS/FAIL/TIMEOUT) |
| is_ap_sta_open | bool | h:71 | AP+STA 复合模式是否开启 |
| first_time_httpdata_request | bool | h:74 | 首联网拉数据标志 |
| update_httpdata_request | bool | h:76 | 下拉刷新标志 |
| setting_city / setting_city_len | char[15] / u8 | h:79-80 | 天气城市 |
| tomato_work_time / rest_time / time_count | u8 ×3 | h:84-86 | 番茄钟参数（持久化） |
| tp_action_manage | TP_ACTION_MANAGE_T | h:89 | {tp_interval_timecount, status, start/stop x/y, tp_action}（h:49-57）手势状态机 |
| tp_wackup_timeleft | u32 | h:91 | 浅睡倒计时（秒） |
| hour / minute / second | u8 ×3 | h:94-96 | 系统时间（SNTP/HTTP 校准，定时器 ISR 走秒） |
| tomato_left_time | u8 | h:98 | **全仓库无任何读写**（孤立字段） |

访问器 32 个（ds_system_data.c:16-298）：整体读 `get_system_data()`（:25，按值返回）；写点：wifi 信息 NV:28/68、状态 AS:34/52/59/64/153-162、请求标志 AM:140/148/165、PM:102、城市 SD:107-114（→save_city NVS）、番茄 SD:132-145（→time_set→NVS）、触摸 SD:148-237、时间 SD:267-298。**持久化字段仅**：ssid/psw/flag、city、番茄三参数（§7 表）；其余易失。关联枚举 TP_ACTION_E（h:13-24）、SETTING_STATUS_E（h:26-31，**无使用者**）。

---

## 10. 死代码 / 测试代码（移植可忽略；“无调用者”均已 grep 确认）

**整文件级**：
- `src/ui/ds_ui_num_display.c`——不在 CMake SRCS，全文注释，无对应头文件。
- `src/net/ds_dns_server.c`——实现全注释，入口为空函数。
- `src/net/ds_wifi_scan.c`、`ds_wifi_ap.c`、`ds_wifi_sta.c`（含事件组）、`ds_https_request.c`（含 8192 栈任务）——入口函数全仓库无调用。
- `src/ds_test.c`——printf 测试函数，无调用。

**函数级（编译但无调用者）**：ds_screen.c demo 链 `test_SSD1681`:902、`EPD_HW_Init`:365、`EPD_SetRAMValue_BaseMap`:411、`EPD_Dis_Part`:435、`EPD_Update`:347、`EPD_Part_Update`:356、`EPD_DeepSleep`:340、`Epaper_READBUSY`:332、`EPD_WhiteScreen_White`:887，及 `ds_screen_partial_display`:151、`ds_screen_init/clean_white`:328/322、`ds_screen_display_white`:109（仅被 init 用）；ds_gpio.c `ds_gpio_init`:161；ds_spi.c `screen_spi_test`:106、`spi_pre_transfer_callback`:72（从未注册）；ds_nvs.c `ds_nvs_test_func`:159、`clean_wifi_info`:77；ds_font.c `test_ds_font`:110；ds_ui_mainpage.c `ds_ui_back_main_page`:17；ds_ui_timepage.c `ds_ui_timepage_num_clear`:44；ds_ui_tomatopage.c `ds_ui_tomatopage_num_clear`:80；ds_system_data.c `get_tp_action`:244、`clear_tp_action`:248；ds_http_request.c `cjson_fans_info`:165、`ds_http_post`:571（函数体全注释）。
**不可达路径**：http_request_task 的 TIME/FANS/CITY 分支（唯一事件源只发 WEATHER，HR:611）；PAGE_TYPE_FANS 页面（主页左滑分支被注释 PM:97-100）；app_main.c:249-260（sleep_mode_init 死循环之后）。
**悬空/无效声明**：ds_screen.h:24 `ds_screen_partial_test` 无定义；ds_data_weather.h:6 `gImage_city_guangzhou[288]` 无定义；FANS_PAGE_T 的 ten/utils 字段、http_server 的 user_id/user_code（:35-36）未使用。
**大块注释**：ds_data_num.c:234-326（24×24 旧数字）、ds_screen.c:855-885（旧 gImage_num1-3）与 914-943、AM:199-200/230-232/256-258、AS:36-45/144-145、HR:365-380/404-416/571-601、SD:180-182、FT:44-49/120-133、GI:95/153-158、TI:53/75、PM:97-100、WP/WD/FA 各自的旧局刷方案段、ds_font.c:121-213（LVGL 注释）。
**测试残留（活代码内）**：ds_ui_fans.c:111-112 `int_to_char(result,12345)` 结果弃用；ds_ui_timepage.c:191-194 开机假数据 1:37/26℃/88%（temperature/humidity 全仓库无其它写点，**时钟页温湿度永远是假值**）；ds_ui_wordpage_init 被调用两次（app_main.c:220-221）；TO:226 向无参函数 `ds_ui_tomatopage_update()`（定义 :91）传参 true（C 旧式声明容忍）；TO:205/212 完成时连续两次 send_beep_event(BEEP_LONG)。

---

## 不确定 / 未找到

1. **ESP-IDF 版本 5.4.1**：来自 sdkconfig `CONFIG_IDF_INIT_VERSION`（子代理引用 sdkconfig:250），我未亲核该行。
2. **TIMER_BASE_CLK 宏值**：本机未找到 IDF 头文件定义；“10ms 周期”依据 alarm 计算（ds_timer.c:24-25）与两处注释（:112、:134）。
3. **蜂鸣器有源/无源**：代码未标注（50% duty + 强制低的驱动方式两可）。
4. **触摸 200→152 缩放**（ds_ft6336.c:78-79）：为何是 152 代码无说明，仅注释 "To 152x152"（推测量为物理可视区，未证实）。
5. **setting.html 内部行号**与 main/html/、main/https_pem/ 目录与 main/ 下同名文件“内容相同”的结论来自子代理（diff 报告为空），handler 侧行为我已亲核，html 行号未逐条复核。
6. **data/ 数组的像素内容**未逐字节验证；尺寸由数组声明字节数、注释与消费代码推得（部分注释本身矛盾，如 size48 实为 32×48、size24 实为 16×24，已按字节数取实）。
7. **ds_data_image.h 的 LUT 数组**：声明为 const 且头文件内定义，但驱动从不发送 LUT 命令——其用途（历史版本？）不确定。
8. **未找到**：gImage_city_guangzhou 的定义；ds_screen_partial_test 的定义；背光/电源使能/实体按键 GPIO（确不存在）；lwip socket 的存活使用（仅注释）；esp_crt_bundle（未使用）；xSemaphore/xTimerCreate/esp_timer_create/gptimer（均无）。
9. 根目录 `example_test.py`、`esptool.py`、`Makefile`、`sdkconfig.old/ci`、`font/` 目录为模板/工具文件，按任务范围未纳入摸底。
10. ` Paint_DrawString_CN` 坐标公式中的 `(en_data_num+cn_data_num)*3` 字间距、`-page_len` 等排版细节的视觉含义未进一步考证（公式已原样给出）。

# 评估方法

优化前后都按这份方法评估。用同一套脚本、同一份测试剧本、同一块板子，结果才能直接对比。脚本在 [tools/perf/](../../tools/perf/)。

## 1. 环境

| 项目 | 值 |
|---|---|
| 芯片 | ESP32-D0WDQ6，revision v1.1，双核，无 PSRAM |
| Flash | 4 MB，DIO，40 MHz |
| 板子 | DesktopScreen：SSD1681 1.54 寸墨水屏（200×200）+ FT6336 触摸 + 蜂鸣器 |
| ESP-IDF | v5.4.1（`~/esp/esp-idf`），工具链 esp-14.2.0_20241119 |
| 主机 | WSL2（Linux 6.18），串口经 usbipd 映射为 `/dev/ttyUSB0` |
| 代码度量 | lizard 1.17.13（脚本自动装到 `~/.cache/desktopscreen-perf/venv`） |

升级 ESP-IDF 或换板子之后，要先在新环境下重测一次基线，再对比优化效果。

## 2. 评估维度和指标

| 维度 | 指标 | 怎么测 | 脚本 |
|---|---|---|---|
| Flash | app.bin 大小和 app 分区余量；bootloader 余量 | 编译日志里 `check_sizes.py` 的输出 | size_report |
| | Flash Code / Flash Data（.text / .rodata） | `esp_idf_size` 解析 map 文件 | size_report |
| | 按模块归类（WiFi、mbedTLS、lwIP、HTTP、C 库、存储、业务代码） | `esp_idf_size --archives` | size_report |
| | 合并字符串池（日志、断言字符串） | 链接器把可合并字符串常量归到第一个输入段，统计时记在 `esp_app_desc.c.obj` 名下 | size_report |
| | 图片数组位置和大小 | `nm` 取 `gImage_*` 符号，按地址区分 Flash（0x3F4xxxxx）和 DRAM | size_report |
| | 分区布局、未分配空间 | `partitions.csv` 加上 `CONFIG_ESPTOOLPY_FLASHSIZE` | size_report |
| | 只改配置能省多少 | 每组改一份 sdkconfig 后全量编译，和 base 比较 | size_variants |
| RAM（静态） | DRAM .data/.bss、IRAM | `esp_idf_size` | size_report |
| RAM（运行时） | 堆总量、进入 app_main 时剩余、空闲阶段剩余、历史最低剩余、最大连续空闲块 | 探针每 5 秒调用 `heap_caps_*` | device_probe |
| | 每个任务的栈大小、最小剩余（高水位） | 探针包装 `xTaskCreatePinnedToCore` 记下栈大小；`uxTaskGetSystemState` 取水位 | device_probe |
| CPU | 按剧本阶段分别统计每个核的负载 | FreeRTOS 运行时间统计：负载 = 1 − IDLE 任务时间占比 | device_probe |
| | 单个 5 秒窗口里的峰值负载；各任务 CPU 时间 | 同上 | device_probe |
| | 周期性活动（定时器、轮询、日志） | 静态审查，见基线文档 | 人工 + AI |
| 功耗 | 代理指标：空闲阶段 CPU 负载、light sleep 次数和时长、WiFi 模式、刷屏次数 | 探针事件 + 静态审查 | device_probe |
| | 实际电流（可选） | 见第 6 节 | 手工 |
| 响应 | 启动时间线：进入 app_main、STA 拿到 IP、对时完成、第一次刷屏、第一次 HTTP 完成 | 探针事件时间戳（以 esp_timer 启动为起点） | device_probe |
| | 每次全刷、局刷、HTTP 请求的耗时 | 探针包装对应函数 | device_probe |
| 代码 | 文件数、物理行、NLOC、注释行 | 自己统计：NLOC 指去掉注释和空行后的行数 | code_metrics |
| | 函数数、平均/最大圈复杂度、CCN > 10/15 的函数、长函数（> 60 NLOC） | lizard | code_metrics |
| | 重复代码率 | `lizard -Eduplicate` | code_metrics |
| | printf / ESP_LOGx 调用次数 | 正则统计（去掉注释后） | code_metrics |
| | 编译告警（去重后，区分项目代码和 IDF） | 全量编译日志 | size_report |
| 架构、健壮性、安全 | 分层依赖、全局状态、并发、越界、输入校验、敏感信息 | 静态审查（Codex 和 ZCode 摸底，Claude 逐条对照源码核实），问题记成 GitHub issue | 人工 + AI |

`main/src/data`（位图数组）不计入 NLOC 和圈复杂度，单独统计行数和字节数。

## 3. 测试剧本

设备实测按 [tools/perf/scenario.txt](../../tools/perf/scenario.txt) 的四个阶段进行。**默认由探针自动执行，不需要人操作**；加 `--manual` 才改为按终端提示人工操作。

| 时间 | 阶段 | 自动执行的操作 | 主要看什么 |
|---|---|---|---|
| 0–60 s | 阶段1 启动 | 无 | 启动时间线、联网、首次刷屏、HTTP |
| 60–120 s | 阶段2 翻页 | 每 16 秒模拟一次"点按进入页面、8 秒后右滑返回主页"，依次是时钟、天气、番茄钟、单词卡 | 全刷耗时、触摸和 UI 任务的栈、刷屏时的 CPU |
| 120–180 s | 阶段3 配网页 | 第 5、20、35 秒各请求一次 `/` 和 `/favicon.ico`，不提交表单 | httpd 栈、堆最低点 |
| 180–300 s | 阶段4 空闲 | 无 | 空闲 CPU、空闲时的堆 |

自动执行的实现（[perf_scenario.c](../../tools/perf/probe/perf_probe/perf_scenario.c)、[perf_touch.c](../../tools/perf/probe/perf_probe/perf_touch.c)）：

- **翻页**：模拟触摸芯片，走被测代码完整的触摸链路。包装 `gpio_isr_handler_add` 拿到 gpio.c 注册的触摸中断处理函数，由探针直接调用来模拟按下和抬起边沿；包装 `gpio_get_level`，模拟期间 INT 脚（GPIO4）返回模拟电平；包装 `i2c_master_read_slave`，按 FT6336 的寄存器格式返回模拟坐标。不驱动任何硬件引脚。点按保持 100 ms；右滑在 200 ms 内把 x 从 30 移到 170，250 ms 时抬起。
- **配网页**：默认（`--http host`）由电脑上的采集脚本通过局域网请求设备的 STA 地址，要求电脑和设备在同一网段；设备 IP 从探针的 got_ip 事件里取，不写进摘要。`--http loopback` 则由设备自己通过 127.0.0.1 请求。

和真人操作的差别（偏差可接受，对比时注意前后用同一种方式）：

- 触摸中断处理函数是在探针任务里调用的，不在真实的中断上下文；I2C 总线传输被跳过，所以 `timer_evt_task` 读坐标比真实情况快一点。
- 没有手机关联 AP、DHCP 分配地址的过程（ESP32 的 STA 已经连着路由器，不能再连自己的 AP；这台电脑靠 WLAN 上网，用电脑无线网卡去连 AP 会让电脑断网）。电脑方式下请求走的是 STA 接口，不是 AP 接口。
- 被测代码的滑动事件 `action` 字段没有初始化（issue #10），右滑返回主页时偶尔可能多刷一次设置页再回主页。
- 自动剧本依赖被测代码的触摸接口、引脚号和主页布局（以 `6f09703` 为准）。重构后这些变了，要同步修改 `perf_touch.c` 和 `perf_scenario.c`；触摸链路不可用时会退回直接发 UI 事件，摘要里"走触摸模拟"的次数会变成 0。

摘要的第一部分会列出自动操作次数、其中走触摸模拟的次数、配网页请求成功次数和刷屏次数，用来核对剧本有没有真正执行。

这份剧本覆盖不到的场景：600 秒后的 AP 关闭和 light sleep（需要至少 11 分钟，而且要停在主页）、番茄钟运行中、配网提交。要评估休眠，就用 `--seconds 900` 加长采集（阶段 4 结束时设备已经回到主页）。

## 4. 运行步骤

```bash
. $HOME/esp/esp-idf/export.sh      # 脚本也会自动加载
bash tools/perf/size_report.sh   --ref <提交> --out perf-out/<名字>
bash tools/perf/code_metrics.sh  --ref <提交> --out perf-out/<名字>
bash tools/perf/size_variants.sh --ref <提交> --out perf-out/<名字>     # 可选
bash tools/perf/device_probe.sh  --ref <提交> --out perf-out/<名字>     # 先关掉 idf.py monitor
python3 tools/perf/compare.py perf-out/baseline perf-out/<名字> > perf-out/<名字>/compare.md
```

`--worktree` 用来评估还没提交的改动。每个脚本在结果目录下生成 `summary.md`（给人看）和 `metrics.json`（给 `compare.py` 用）。

`device_probe.sh` 的流程：

1. 导出被测代码，追加 [probe/sdkconfig.probe](../../tools/perf/probe/sdkconfig.probe)（打开 FreeRTOS 运行时间统计），把探针组件链接进去，编译出测量固件。
2. 读设备的分区表，和被测代码的分区表比较；不一致就停止。分区表改过的版本，要先用 `idf.py flash` 完整烧一次。
3. 分块备份设备当前的 app 分区，用 `image_info` 校验备份。
4. 只写 app 分区，复位后采集 300 秒，剧本自动执行。复位后 15 秒内没看到探针输出（比如芯片进了下载模式）就提前结束，写回原固件并提示拔插 USB。
5. 写回备份并复位，核对启动日志里的 App version 和备份一致。
6. 解析日志。

## 5. 探针原理和开销

探针组件在 [tools/perf/probe/perf_probe/](../../tools/perf/probe/perf_probe/)，只链接进测量固件。

- 用链接器 `--wrap` 截获 `app_main`，在进入 `app_main` 时创建采样任务（优先级 1，栈 4 KB）。被测代码不需要改动。
- 包装 `xTaskCreatePinnedToCore` 和 `xTaskCreatePinnedToCoreWithCaps`（httpd 用后者，第 2 版起才包装）记下每个任务的栈大小。IDLE 和 Tmr Svc 是 FreeRTOS 内部创建的，栈大小从 sdkconfig 读。
- 包装 `ds_screen_full_display`、`ds_screen_full_display_bydata`、`ds_screen_partial_data_copy`、`ds_screen_refresh`、`esp_http_client_perform`、`esp_light_sleep_start`，记录耗时；监听 `IP_EVENT_STA_GOT_IP` 和 `WIFI_EVENT_AP_STACONNECTED`；轮询 `time()` 判断对时完成。只有跨文件的调用会经过包装函数：screen.c 内部对 `ds_screen_refresh` 的调用不计入，所以 `screen_refresh` 统计的是时钟局刷。
- 包装函数只把事件写进环形缓冲，**不调用 printf**，免得改变被测任务的栈水位。所有输出都由采样任务完成，而且只用 32 位整数格式化，开启 newlib nano 格式化以后也能正常输出。
- CPU 时间以 esp_timer 的微秒计数。中断时间算在被打断的任务头上。light sleep 期间所有任务都不计时，会体现为"未计入"。
- 开销：测量固件比正式固件大约 4 KB；探针占用 808 B 静态内存，加上 8,112 B 堆（4 KB 任务栈、任务控制块、事件和采样缓冲区；第 2 版起在 hello 行里如实输出）。摘要里同时给出"扣除探针开销"的估算值。打开运行时间统计后，每个任务控制块会多几个字节。
- 探针固定在 core1（第 2 版起），它在 UART 上忙等的时间会从 core1 负载里扣掉；第 1 版没有绑核，看"业务负载"一列。
- 主机侧串口偶尔丢字节，所以每组采样都带校验和（第 2 版起），对不上的整组丢弃，摘要里会列出丢了几组。运行时间是累积计数，丢一组不影响 CPU 统计。

## 6. 功耗怎么评估

还没有电流表数据，目前用代理指标：

- 阶段 4（空闲）每个核的 CPU 负载。负载越低，能降频或进入睡眠的空间越大。
- light sleep 的次数和时长（标准剧本 300 秒内不会进入，见第 3 节）。
- 空闲时仍然周期唤醒的活动，见基线文档里的周期性活动表。
- WiFi 模式：开着 APSTA 时，STA 的 modem sleep 不起作用。

需要实测电流时，用 USB 电流表或功率计串在供电上，**用正式固件而不是测量固件**，按同一剧本记录每个阶段的平均电流；再停在主页等它进入 light sleep，记录睡眠电流和唤醒周期。结果补进对应日期的评估文档。

## 7. 注意事项

- **不要在仓库根目录运行 `python -m esptool`。** 根目录有一份旧的 `esptool.py`（v2.9），`python -m` 会优先导入它。它不锁串口，会在 monitor 开着的时候把芯片复位进下载模式。脚本里一律在结果目录下调用 IDF 自带的 `esptool.py`。
- WSL + usbipd 下，460800 波特率读 1 MB 会报 `Corrupt data`，所以脚本默认用 230400、每次读 256 KB，失败重试。
- WSL 重启或 USB 重新挂载后要重新 `usbipd attach --wsl --busid <BUSID>`，串口节点默认是 `root:dialout 660`，要 `sudo chmod 666 /dev/ttyUSB0`（或者加一条 udev 规则 `KERNEL=="ttyUSB[0-9]*", MODE="0666"`）。
- 偶尔会遇到 CH340 的 DTR 卡住：每次 RTS 复位后芯片都进下载模式（日志里是 `boot:0x5` 和 `waiting for download`），连打开串口都会触发一次。拔插一次 USB 就能恢复。脚本检测到这种情况会提前结束并提示。
- 运行前关掉占用串口的 `idf.py monitor`，脚本检测到串口被占用会直接退出。
- `device/probe.log` 是原始串口日志，基线固件会把 WiFi 密码打印出来，所以只留在 `perf-out/`，不要提交。摘要和 metrics.json 里不含原始日志。
- 写回的是设备上原来的固件，不一定是 HEAD。基线评估那天设备上跑的是 `v2.6-20-g8bcb746`。
- 探针的时间戳以 esp_timer 启动为起点，约比芯片复位晚 0.5 秒（bootloader 加载 app 的时间不含在内）。

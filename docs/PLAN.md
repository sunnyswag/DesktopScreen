# DesktopScreen 多平台移植路径（ESP-IDF / NuttX / Zephyr）

> 依据：[survey-desktopscreen.md](survey-desktopscreen.md)（ZCode 摸底原项目）、[survey-nuttx.md](survey-nuttx.md)（Codex 摸底 NuttX）、[survey-zephyr.md](survey-zephyr.md)（Codex 摸底 Zephyr v4.5.0），关键结论已抽查原文核实。
> 制定：2026-10-31；同日修订：从"单独迁移到 NuttX"改为"本仓库通过平台抽象层同时适配多个系统"

## 修订：仓库与依赖管理（优先于下文中与之冲突的内容）

- **只有一个 Git 仓库，就是 DesktopScreen。** 原先单独建的 `desktopscreen-nuttx` 仓库已废弃，文档挪到本仓库的 `docs/`，脚本挪到 `tools/nuttx/`（阶段 B 要按下面的 repo 方式重写）。
- **平台抽象层隔离三个系统**，目标结构（阶段 P 落实；在那之前 ESP-IDF 工程仍在仓库根目录，不影响现在的编译）：

  ```
  DesktopScreen/
  ├── core/            # 纯 C：paint、字库、页面、手势识别，不依赖任何系统
  ├── pal/             # 平台抽象层接口（头文件）：显示、触摸、存储、网络、时间、蜂鸣
  ├── ports/
  │   ├── esp-idf/     # 现在根目录下的 IDF 工程挪到这里，实现 pal
  │   ├── nuttx/       # 自定义板 + 应用 + 补丁 + repo manifest
  │   └── zephyr/      # Zephyr 应用 + 板 overlay + west.yml
  ├── tools/  docs/
  └── vendor/          # repo / west 拉下来的依赖，.gitignore 忽略
  ```
- **依赖管理**：NuttX 和 nuttx-apps 用 **repo** 管理（manifest 放在本仓库，拉到 `vendor/`）；Zephyr 和它的模块用 **west** 管理（`west.yml` 放在本仓库）；ESP-IDF 不进 vendor，继续用 `~/esp/esp-idf` v5.4.1，在文档里写明版本。repo 和 west 具体怎么在本仓库里落地（manifest 位置、工作区根目录），到对应阶段开工时先验证再定。
- **顺序**：A（学原项目）→ **P（新增：抽出 core + pal，先在 ESP-IDF 上跑通，对照原版行为）** → NuttX（B、C、E、F）→ Zephyr。下文的阶段 D 并入 P，NuttX 端只需要写 port。NuttX 在前、Zephyr 在后：NuttX 是原定目标（开发记录里的视频选题），也和你熟悉的 Vela/repo 体系一致；Zephyr 放在后面，正好检验 pal 抽得干不干净。Zephyr 的阶段见第 5 节。

## 0. 先说结论

- **难度比想象中低**：原项目约 40% 是死代码。NuttX 已有 SSD1681 200×200 驱动、FT5x06 触摸驱动（寄存器和 FT6336 兼容）、ESP32 WiFi STA+AP、webclient、cJSON、ntpclient、dhcpd，还有 ESP32 墨水屏板级接入参考（`ttgo_eink5_v2:fb`）。
- **真正要自己写的**：板级 bringup、平台适配层、一个小 HTTP 配置服务器、配置存储、墨水屏**局部刷新**（NuttX 的 ssd1680 驱动 `redraw` 只做全刷，见 `drivers/lcd/ssd1680.c:389`）。
- **不用迁移的**：captive DNS（原项目里本来就注释掉了）、HTTPS（原项目只用 HTTP 请求天气）、粉丝页（入口被注释，到不了）、WiFi 扫描。

## 1. 已定的技术选型

| 决策 | 选择 | 理由 |
|---|---|---|
| NuttX 源码树 | **上游 Apache NuttX + nuttx-apps，两者切到同一个 release tag**（本地已有 `nuttx-12.10.0`；可以先 fetch 看有没有更新的） | ESP32 BSP 由 Espressif 在上游维护，文档也对得上。`~/Git/nuttx` 已经是上游，只缺 apps。Vela 构建体系不同，只当参考（kvdb、curl） |
| 启动方式 | Simple Boot（默认，不需要单独的 bootloader） | 少一个环节；以后要 OTA 再换 MCUboot |
| 板子 | 自建 out-of-tree 板 `desktopscreen`，从 `esp32-devkitc` 复制，芯片选 `ESP32WROOM32`（4MB、无 PSRAM） | 不污染 nuttx 树，升级时方便 |
| 应用 | out-of-tree app，软链接到 `apps/external/desktopscreen` | 同上 |
| 显示 | 第一步用 NuttX `ssd1680` 驱动 + `/dev/fb0`（全刷）；局刷后面再给驱动加 | 先学会 NuttX 的 LCD/fb 框架，再改驱动 |
| UI | **先保留原来的 Paint/字库/页面代码**，只换底层；LVGL 作为可选的后期项目 | 先把迁移跑通，一次只换一层 |
| 配置存储（替代 NVS） | LittleFS 挂到 `/data`，配置写成一个 `config.json`（用 cJSON 读写） | 原项目只有 3 组共 7 个 key，没必要做 NVS 兼容层 |
| 字库 | 原样的二进制，放在 Flash 裸区，板级注册成 MTD，应用用 `pread` 读 | `ds_font.c` 只需要按偏移读，改动最小 |
| 定时/并发 | 不用硬件定时器；改用 `poll()` + 超时、POSIX mqueue、`clock_gettime`；系统时间交给 NuttX 时钟 + ntpclient | 原项目的 10ms 定时器 ISR 有 bug（ISR 里调用非 IRAM 函数、往队列发未初始化的结构） |
| 蜂鸣器 | MCPWM 换成 LEDC（`/dev/pwm0`，GPIO15） | NuttX 的 ESP32 有 LEDC 驱动 |
| HTTP 配置服务 | 用 BSD socket 自己写一个最小服务器：GET `/` + 4 个 POST JSON 接口 | `netutils/webserver` 不擅长处理 POST JSON，thttpd 依赖重；自己写约 300 行，也正好练手 |
| 时区 | 应用里固定加 8 小时（不启用 zoneinfo） | 省掉 romfs tz 数据库 |

### 建议的 Flash 布局（4MB，第 2 阶段落实）

| 区域 | 偏移 | 大小 | 说明 |
|---|---|---|---|
| app（Simple Boot） | 0x001000 | ~2.3MB | 带 WiFi 的 NuttX 镜像比 IDF 版大，留足余量 |
| LittleFS `/data` | 0x250000 | 256KB | config.json + 上传的文件 |
| 字库裸区 `/dev/font` | 0x290000 | 0x170000 | myFont1.bin 实际 1,480,385 字节，放得下；**要用 esptool 重新烧到新偏移** |

## 2. 阶段路线

每个阶段都有可以检查的验收标准，做完一个再进下一个。⭐ 表示你自己动手学习的重点。

### 阶段 A：带着迁移视角把 DesktopScreen 学完（1–3 天）

**已完成**：死代码已在 [DesktopScreen](https://github.com/sunnyswag/DesktopScreen) 的 `main` 分支删除（`50e5eee`），并且已适配 ESP-IDF 5.4.1、能编译通过（`af14cd9`）。粉丝页 `ds_ui_fans.c` 还在，但入口被注释掉，可以跳过。

**注意行号**：下面以及两份摸底报告里的行号，都是按原始提交 `e18432a` 算的（用 `git show e18432a:main/<路径>` 查看）；main 分支删了代码，行号已经变了，按函数名找即可。

**按这个顺序读**：
1. `app_main.c:178-261`：初始化顺序，以及 `sleep_mode_init` 的浅睡循环
2. 触摸链：`ds_gpio.c:42-62` → `ds_timer.c:114-128` → `ds_system_data.c:177-238`（手势识别）→ `ds_ui_page_manage.c:63-124`（切页）
3. 墨水屏：`ds_screen.c:17-256`（init 序列、全刷、局刷、BUSY）⭐ 迁移时要对照 NuttX 的 `ssd1680.c` 读
4. 字库：`ds_font.c` 全文 + `ds_paint.c:436-490`（Unicode、4bpp、XBF 格式）
5. 网络：`ds_wifi_ap_sta.c`、`ds_http_server.c:409-552`（4 个 POST 接口）、`ds_http_request.c:483-494, 117-144`（天气）、`ds_sntp.c`
6. `ds_nvs.c`：7 个 key

**边读边记**：读每个模块时写一句"它在 NuttX 上对应什么"，可以直接对照第 3 节的映射表。
**验收**：能不看代码画出"触摸 → 手势 → 切页 → 刷屏"和"配网 → 连网 → 拉天气/校时"这两条链。

### 阶段 B：NuttX 环境 + 基础（2–4 天）⭐

1. 环境：
   - `~/Git/nuttx` 有未跟踪的 `CMakeCache.txt`、`CMakeFiles/`（以前用 CMake 构建留下的），先清掉或忽略
   - 切到 release tag：`git checkout nuttx-12.10.0`（或更新的）
   - clone 同 tag 的 apps：`git clone -b nuttx-12.10.0 https://github.com/apache/nuttx-apps ~/Git/apps`
   - 装 `pip install kconfiglib`（或 `apt install kconfig-frontends`）
   - 把 `~/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin` 加入 PATH，用 IDF 的 python 环境提供 esptool（4.10.0，满足 ≥4.8）
   - 首次构建 ESP32 会下载 `esp-hal-3rdparty`，需要联网
2. 模拟器：`./tools/configure.sh -l sim:nsh` → `make` → `./nuttx`；再跑 `sim:lvgl_fb`，确认 WSLg 能弹出 X11 窗口
3. 上板：`esp32-devkitc:nsh`（menuconfig 里把芯片改成 WROOM32）→ `make flash ESPTOOL_PORT=/dev/ttyUSB0` → 串口进入 `nsh>`
4. 用现成的 defconfig 依次熟悉各项功能：`wifi`（`wapi` 连网、`ping`）、`sta_softap`、`pwm`（把引脚改成 15，听到蜂鸣）、`spiflash`（LittleFS 挂载）、`ttgo_eink5_v2:fb`（看 ssd1680 怎么接到板级）
5. 同时看的文档：NuttX 文档里的 Applications/nsh、Components/drivers（lcd、input、mtd）、platforms/xtensa/esp32

**验收**：sim 和板子都能进 `nsh>`；板子能连上家里 WiFi 并 ping 通外网；GPIO15 能响。

### 阶段 C：自建板级 `desktopscreen`（3–5 天）⭐

引脚表（来自原项目，已核实）：

| 功能 | 引脚 / 参数 |
|---|---|
| SPI2（HSPI） | CLK 25、MOSI 26、MISO 23；mode 0；原项目 15MHz（NuttX 驱动默认 5MHz，先用默认） |
| 屏幕 | CS 27（软件 CS）、DC 14、RST 12、BUSY 13（1 表示忙） |
| I2C1 | SDA 33、SCL 32，100kHz；FT6336 地址 0x38 |
| 触摸 | INT 4（双沿）、RST 5（低电平复位） |
| 蜂鸣器 | GPIO15，4kHz，占空比 50% |

在 bringup 里注册：`/dev/lcd0` + `/dev/fb0`（ssd1680，`CONFIG_LCD_SSD1681_1_54`）、`/dev/input0`（ft5x06）、`/dev/pwm0`（LEDC）、LittleFS `/data`、`/dev/font`（`esp32_spiflash_alloc_mtdpart`）、WiFi。

**验收**（全部用 nsh 和现成的 example 检查）：
- `fb` 示例能在墨水屏上画出图案
- `tc` 触摸示例能打印坐标，四个角的坐标都对
- `pwm` 能响
- `/data` 读写后重启，数据还在
- `xxd` 读 `/dev/font` 的前 16 字节，和 `myFont1.bin` 一致

**风险**：FT6336 和 ft5x06 驱动不一定完全兼容（NuttX 驱动从寄存器 0x01 连读 32 字节）。不行的话，照着 ft5x06.c 写一个精简的 ft6336 驱动。

### 阶段 D：应用骨架 + 平台抽象层，先在 sim 上跑通 UI（4–6 天）

- 分成两层：**core**（paint、font、页面、图片数据、手势识别，纯 C，不碰平台）和 **port**（平台接口）
- port 接口（具体签名在本阶段开工时由我定）：`display_full(buf)`、`display_partial(x,y,w,h,buf)`、`touch_wait(timeout)`、`font_read(off,buf,len)`、`cfg_get/set`、`beep(type)`、`now()`
- 先写 sim 实现（fb 窗口 + X11 鼠标模拟触摸 + 从文件读字库），再写 esp32 实现
- 主循环改成：一个 UI 线程 `poll(touch_fd, 1s)`，超时就处理时钟和番茄钟的计时；网络单独一个线程，通过 mqueue 通知 UI
- 搬代码时顺手修掉原项目的已知 bug（摸底报告第 4 节、第 10 节）：未初始化的事件字段、在任务上下文里用 `xQueueSendFromISR`、`wordpage_init` 调用两次、温湿度假数据

**验收**：sim 上所有页面都能显示，中文正常，点击和滑动能切页，番茄钟能计时。

### 阶段 E：网络（4–6 天）

1. STA：开机读 `config.json` → `wapi` 库连网 → DHCP 拿 IP
2. AP+STA 配网：SoftAP（SSID "桌面屏幕"）+ `dhcpd` + 自写 HTTP 服务器：GET `/` 返回内嵌的 `setting.html`，POST `/wifi_data`、`/tomato_data`、`/city_data`、`/back`，接口格式和原项目一致，这样网页可以原样复用
3. 天气：webclient GET 心知天气 + cJSON 解析，4 个字段
4. 校时：ntpclient（pool.ntp.org），+8 小时
5. 原项目的行为：配网 AP 10 分钟后自动关闭、下拉刷新触发拉天气和校时

**验收**：手机连上 AP，打开 192.168.4.1 填 WiFi，设备连网后显示正确的天气和时间；重启后自动连网。

### 阶段 F：上板整合 + 墨水屏局刷（3–5 天）⭐

- 给 `ssd1680.c` 加局部刷新：参照原项目 `ds_screen.c:204-256` 的窗口设置和 RST 脉冲，通过 ioctl 或 `putarea` 加一个 redraw 模式。这一步是学 NuttX 内核驱动开发的好练习
- 备选：驱动改起来卡住的话，就在用户态用 `/dev/spiN`（`CONFIG_SPI_DRIVER`）+ GPIO 直接照搬 `ds_screen.c`
- 刷新节流照原项目来：局刷超过 5 次后做一次全刷
- 拿原项目当行为基准，逐条比对：开机流程、每个页面、手势、配网、天气、校时、蜂鸣

**验收**：功能比对清单全部通过；时钟页每分钟局刷不闪屏。

### 阶段 G（可选）

低功耗（NuttX ESP32 的 PM/autopm 替代 light sleep，难度最高）、用 LVGL 重写 UI（可以先在 sim 上做）、MCUboot + OTA、HTTPS（mbedtls + webclient tls_ops）。

## 3. API 映射速查

| ESP-IDF / FreeRTOS | NuttX |
|---|---|
| `xTaskCreate` | `pthread_create` / `task_create` |
| `xQueue*` | `mq_open/mq_send/mq_receive`；单个生产者也可以用 pipe |
| GPIO ISR + 队列 | 驱动处理中断，应用 `poll()` 设备文件 |
| `spi_device_polling_transmit` | 内核驱动：`SPI_SEND/SPI_SNDBLOCK`；用户态：`/dev/spiN` + `SPIIOC_TRANSFER` |
| `i2c_cmd_link_*` | 内核：`I2C_TRANSFER`；用户态：`/dev/i2cN` |
| `mcpwm_*` | `/dev/pwm0` + `PWMIOC_SETCHARACTERISTICS/START/STOP` |
| `timer_group` ISR（10ms） | `poll` 超时 / `timerfd` / `clock_nanosleep` |
| `nvs_*` | `/data/config.json` + cJSON |
| `esp_partition_read` | `open("/dev/font")` + `pread` |
| `esp_vfs_spiffs_register` | LittleFS 挂载 `/data` |
| `esp_wifi_*` / `esp_netif` | `wapi` 库 / `netlib_*`，SoftAP 用 `CONFIG_ESP32_WIFI_STA_SOFTAP` |
| `esp_http_client` | `netutils/webclient` |
| `esp_http_server` | 自写 socket 服务器 |
| `esp_sntp` | `netutils/ntpclient`（`ntpc_start`） |
| `esp_light_sleep_start` | NuttX PM 框架（阶段 G） |

## 4. 分工：Codex / ZCode 能帮你做什么

规划、接口设计、取舍由我（Claude）负责；按规格写代码、审查、找资料交给执行者；⭐ 项建议你自己动手。

| 阶段 | 执行者可以做的 | 谁 |
|---|---|---|
| B | 写 `tools/setup.sh`（拉取 nuttx/apps、建 venv）和 `tools/env.sh`（PATH、常用 configure/flash 命令） | ZCode（2026-10-31 已派） |
| C | 我定好引脚和设备清单后，按规格写 board 目录（bringup、Kconfig、defconfig） | Codex 写，ZCode 审 |
| D | 我定好 port 接口后，把 paint/font/页面代码机械地搬到新接口上，删掉死代码；写 sim 端的 port 实现 | Codex（搬核心代码）+ ZCode（删死代码、做 sim port） |
| E | 按我写的路由规格实现最小 HTTP 服务器、配置读写、天气解析 | Codex 写，ZCode 审 |
| F | 对照 `ds_screen.c` 和 `ssd1680.c` 列出局刷需要改的点；写行为比对清单 | Codex（只读分析）、ZCode（清单） |
| 全程 | 遇到疑难 bug 时给第二意见；审查大 diff | Codex |

**省 Claude 额度的做法**：每个阶段开一个新会话，开头说"按 `~/esp/DesktopScreen/docs/PLAN.md` 做阶段 X"。我只读这份计划和相关的摸底章节，然后写规格、派发、验收，不在旧会话里堆上下文。含写入的派发会先给你看规格，你确认后才开始。

## 5. Zephyr 部分（v4.5.0，依据 survey-zephyr.md）

### 和 NuttX 比，Zephyr 现成的更多

| 项目 | Zephyr | NuttX |
|---|---|---|
| 墨水屏局刷 | **现成**：`ssd16xx` 驱动带 full/partial 两套 profile（参考 shield `waveshare_epaper_gdeh0154a07`） | 要自己给 ssd1680 驱动加 |
| FT6336 | `focaltech,ft5336` 驱动，树内已有 FT6336 实例（M5Stack Core2 等） | ft5x06，兼容性未确认 |
| 键值存储 | Settings + NVS/ZMS，原生支持 | 用 config.json 自己写 |
| 字库分区 | 在 devicetree 里定义分区，用 `flash_area_read` 读 | 板级注册 MTD |
| 配网 | APSTA 示例 + DHCPv4 服务器 + HTTP 服务器（支持动态 POST，标注 EXPERIMENTAL） | 自己写 HTTP 服务器 |
| 低功耗 | 有 light sleep 示例（RTC 定时唤醒确认可用；GPIO4 唤醒有代码，未验证） | 最难，放在阶段 G |
| 缺的 | 没有 cJSON；没有 SPIFFS | — |

代价是学习曲线更陡：要学 devicetree、Kconfig、west、CMake 这一整套。

### 对 pal 设计的约束（阶段 P 定接口时必须满足）

- **显示**：core 用统一的帧缓冲（沿用原项目格式：横向 1bpp、MSB first、每行 25 字节），由各 port 自己转换。Zephyr 的 ssd16xx 只接受 `MONO01`（0=黑）且是 **VTILED**（每字节纵向 8 个像素），y 方向要 8 像素对齐。刷新接口要带"全刷/局刷"参数：Zephyr 端全刷用 `blanking_on` → `display_write` → `blanking_off`，局刷直接 `display_write`。"局刷 5 次后全刷一次"的策略放在 core。
- **触摸**：pal 只上报按下、抬起和坐标，手势识别放在 core。Zephyr 的 FT5336 驱动只在单边沿触发中断，经 input 回调上报，不能照搬原项目的双沿中断逻辑。
- **JSON**：天气解析属于业务逻辑，放 core；cJSON 作为第三方源码放进 `core/third_party/`，三个平台共用同一份，不依赖各平台自带的版本。
- **时间**：core 自己处理 +8 时区。Zephyr 默认的 minimal libc 不支持 TZ，NuttX 也说好不启用 zoneinfo。
- **线程与队列**：core 不直接调用任何 OS API，只提供"处理事件 / 定时 tick"两个入口；线程、队列、定时器都由各 port 实现。

### 环境上要注意

- **工具链**：用 Zephyr SDK 1.0.1（`xtensa-espressif_esp32_zephyr-elf`），不复用 IDF 的 GCC（官方不支持）。
- **esptool 版本冲突**：Zephyr 要求 `esptool>=5.0.2`，NuttX 12.10 要的是 `esptool.py`（4.x，5.0 起改名了）。**NuttX 和 Zephyr 各用一个 venv**。
- **west 工作区放哪**：按"修订"一节，依赖拉到本仓库的 `vendor/` 下。west 的工作区根目录是 manifest 所在目录的上一级，`west.yml` 放在 `ports/zephyr/` 时根目录会落在 `ports/`。阶段 Z0 先实测：`west.yml` 放在哪、用 `path-prefix: vendor` 能不能让依赖落到 `vendor/`。验证完再定。
- 现在 `~/Git/zephyrproject/zephyr` 是浅克隆，只用来摸底。正式环境以 Z0 的方案为准，到时候可以删掉。

### Zephyr 阶段

| 阶段 | 内容 | 验收 |
|---|---|---|
| Z0 | west 工作区（按上面的方案）、Zephyr SDK、`west blobs fetch hal_espressif`；先跑 `native_sim` 的 hello_world，再上板跑 `esp32_devkitc/esp32/procpu` 的 hello_world | 模拟器和板子都能打印 |
| Z1 | 在 `native_sim` 上跑 core（SDL 显示 + SDL 触摸），实现 Zephyr 的显示和触摸 port | PC 上所有页面能显示，手势能切页 |
| Z2 | 板级 overlay：SPI2/I2C1/GPIO 引脚、ssd1681（带 full/partial 节点）、ft5336、LEDC 蜂鸣器、Flash 分区（含字库区）；Settings + NVS | 上板 UI 正常，时钟页局刷不闪，配置重启后还在 |
| Z3 | 网络：参考 apsta_mode 示例做配网，HTTP 服务器提供 4 个 POST 接口，HTTP 客户端拉天气，SNTP 校时 | 和 NuttX 阶段 E 的验收一样 |
| Z4（可选） | light sleep：RTC 定时唤醒 + GPIO4 唤醒（要实测）；处理 FT5336 自身休眠和 WiFi PM 阻止睡眠的问题 | 空闲时进入浅睡，触摸能唤醒 |

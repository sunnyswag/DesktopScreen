> 行号基于 DesktopScreen 原始提交 `e18432a`（清理前）。main 分支已删除死代码，行号已变。

本次仅做本机只读检查，未联网、修改文件、运行配置、编译或烧录。“现成”表示源码已有相应实现或构建接入，不表示已在目标硬件验证。

下文路径缩写：

- **N**＝[/home/huiqin123/Git/nuttx](/home/huiqin123/Git/nuttx)
- **V**＝[/home/huiqin123/Git/vela-opensource](/home/huiqin123/Git/vela-opensource)
- **A**＝`V/apps`
- **E**＝`N/arch/xtensa/src/esp32`
- **B**＝`N/boards/xtensa/esp32`

**1．源码树身份、apps 与 Vela 目录**

- **N 是配置为 Apache 上游 master 的独立 checkout。**远端为 `git@github.com:apache/nuttx.git`，分支跟踪 `origin/master`。HEAD 为 `af0aa78cbcf323ab66dab7958ece240773302b23`，提交日期 **2025-02-15**；本地 `git describe` 确实返回 `nuttx-8.2-22214-gaf0aa78cbc`。出处：[N/.git/config:6](/home/huiqin123/Git/nuttx/.git/config:6)、[N/.git/HEAD:1](/home/huiqin123/Git/nuttx/.git/HEAD:1)，以及 N 中只读 `git log -1`、`git describe` 输出。
- **不能把它认定为 NuttX 8.2，也不能仅凭 `.version` 认定为 12.10.0 正式版。**`.version` 写 `12.10.0`、build `af0aa78cbc`；但版本脚本在 describe 结果不是三段版本号时，会改取本地最高版本标签。当前源码已有 12.8.0 发布说明；本地 12.9.0、12.10.0 标签指向的提交分别为 2025-04-11、2025-06-27，均晚于 HEAD。准确描述是：**含 12.8.0 发布说明的 2025-02 开发快照；精确对应哪个发行版本不确定。**出处：[N/.version:3](/home/huiqin123/Git/nuttx/.version:3)、[N/tools/version.sh:75](/home/huiqin123/Git/nuttx/tools/version.sh:75)、[12.8.0 发布说明](/home/huiqin123/Git/nuttx/Documentation/ReleaseNotes/NuttX-12.8.0:1)，N 中对应标签的 `git log -1` 输出。
- **V 自带另一份 NuttX。**`V/nuttx` 的远端是 `open-vela/nuttx`，`V/apps` 的远端是 `open-vela/nuttx-apps`。本地 manifest 将二者列为同一工作区项目。它们才是这里明确配套的一组。出处：[V/nuttx/.git/config:4](/home/huiqin123/Git/vela-opensource/nuttx/.git/config:4)、[V/apps/.git/config:4](/home/huiqin123/Git/vela-opensource/apps/.git/config:4)、[manifest:8](/home/huiqin123/Git/vela-opensource/.repo/manifests/openvela.xml:8)、[manifest:184](/home/huiqin123/Git/vela-opensource/.repo/manifests/openvela.xml:184)。
- **未确认到 N 的配套 Apache apps。**其默认查找位置 `~/Git/apps`、`~/Git/nuttx-apps`、`~/Git/nuttx-apps.git`、`~/Git/apps-12.10.0` 均不存在；N 当前也没有 `.config` 指定其他 apps。`V/apps` 可以作为本机组件调查来源，但与 N 的构建兼容性**不确定**。出处：上述目录只读检查；[N/tools/configure.sh:297](/home/huiqin123/Git/nuttx/tools/configure.sh:297)。
- **`V/packages/apps` 不是原生 NuttX apps 的替代目录。**它属于 openvela JS 应用项目；manifest 将其单独列为 `packages_apps`。出处：[packages/apps/README_zh-cn.md:5](/home/huiqin123/Git/vela-opensource/packages/apps/README_zh-cn.md:5)、[manifest:206](/home/huiqin123/Git/vela-opensource/.repo/manifests/openvela.xml:206)。

V 的主要目录实际为 `nuttx/`、`apps/`、`external/`、`frameworks/`、`packages/`、`vendor/`、`prebuilts/`、`docs/`、`tests/`。`apps/external`、`apps/frameworks`、`apps/packages`、`apps/vendor` 分别链接到同级目录；因此以下 KVDB、UnQLite 等属于 Vela 工作区的附加组件。出处：[V 目录](/home/huiqin123/Git/vela-opensource)、[A 目录](/home/huiqin123/Git/vela-opensource/apps)。

`V/build.sh` 是 `nuttx/tools/build.sh` 的软链接。脚本接受：

```text
./build.sh <board>:<config> [make 参数]
./build.sh <vendor 下的配置目录> [make 参数]
./build.sh <配置> menuconfig
./build.sh <配置> --cmake [-b 输出目录] -j8
```

默认 Make 路径调用 `configure.sh`、`make`、`savedefconfig`，并可能将结果复制回原 defconfig；CMake 路径支持独立输出目录。以上仅为脚本用法，未执行。出处：[V/nuttx/tools/build.sh:309](/home/huiqin123/Git/vela-opensource/nuttx/tools/build.sh:309)、[:348](/home/huiqin123/Git/vela-opensource/nuttx/tools/build.sh:348)、[:450](/home/huiqin123/Git/vela-opensource/nuttx/tools/build.sh:450)。

**2．ESP32 板级配置与 esp32-2432S028**

`B/` 下有 **12 个板级目录、102 份 defconfig**；`common/` 没有 defconfig。下表每个名称对应该行链接目录中的 `<配置名>/defconfig`。

| 板子／出处目录 | 全部配置名 |
|---|---|
| [esp32-2432S028/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-2432S028/configs) | lvgl、nsh |
| [esp32-audio-kit/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-audio-kit/configs) | audio、nsh、wifi |
| [esp32-devkitc/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/configs) | audio、autopm、ble、blewifi、bmp280、brickmatch、buttons、capture、coremark、cxx、dac、efuse、elf、espnow、i2schar、knsh、leds、max6675、mcp2515、mcuboot_nsh、mcuboot_slot_confirm、mcuboot_update_agent、modbus、module、motor、mqttc、ms5611、nsh、nxdiag、nxlooper、oneshot、ostest、pm、psram、psram_usrheap、pwm、qemu_openeth、qencoder、random、rmt、rtc、smp、softap、sotest、spiflash、sta_softap、tickless、timer、twai、wamr_wasi_debug、watchdog、watcher、wifi、wifi_smp、wifinsh、wifishare |
| [esp32-ethernet-kit/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-ethernet-kit/configs) | autopm、buttons、ethernet、nsh、oneshot、rtc、wifi |
| [esp32-lyrat/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-lyrat/configs) | audio、buttons、mmcsdspi、nsh、nxrecorder、rtptools、wifi |
| [esp32-pico-kit/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-pico-kit/configs) | nsh |
| [esp32-sparrow-kit/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-sparrow-kit/configs) | mqttc、nsh、wifi |
| [esp32-wrover-kit/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-wrover-kit/configs) | autopm、bmp180、buttons、gpio、lcd1602、leds、lua、lvgl、mmcsdspi、nsh、nx、oneshot、rtc、wifi |
| [lilygo_tbeam_lora_gps/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/lilygo_tbeam_lora_gps/configs) | gps、nsh、sx127x |
| [ttgo_eink5_v2/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/ttgo_eink5_v2/configs) | fb |
| [ttgo_lora_esp32/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/ttgo_lora_esp32/configs) | nsh、sx127x |
| [ttgo_t_display_esp32/configs](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/ttgo_t_display_esp32/configs) | lvgl_fb、lvgl_lcd、nsh |

**esp32-devkitc 没有名为 `lvgl`、`ili9341`、`st7789` 的配置。**ILI9341 实例在 `esp32-wrover-kit:lvgl`、`esp32-2432S028:lvgl`；ST7789 实例在 `ttgo_t_display_esp32:lvgl_fb/lvgl_lcd`。出处：上述配置目录；[Wrover lvgl:42](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-wrover-kit/configs/lvgl/defconfig:42)、[TTGO lvgl_lcd:40](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/ttgo_t_display_esp32/configs/lvgl_lcd/defconfig:40)。

显式包含 `CONFIG_ESP32_WIFI=y` 的 **26 份配置**如下；冒号后的数字是该配置 `defconfig` 的行号，目录同上表：

| 板子 | 已显式启用 WiFi 的配置及行号 |
|---|---|
| esp32-devkitc | audio:55、autopm:30、blewifi:40、efuse:44、espnow:36、mcuboot_update_agent:32、mqttc:29、nxlooper:53、softap:31、sta_softap:32、wamr_wasi_debug:46、wifi:33、wifi_smp:37、wifinsh:30、wifishare:31 |
| esp32-audio-kit | audio:57、wifi:36 |
| esp32-ethernet-kit | autopm:29、wifi:28 |
| esp32-lyrat | audio:54、rtptools:54、wifi:36 |
| esp32-sparrow-kit | mqttc:59、wifi:62 |
| esp32-wrover-kit | autopm:30、wifi:29 |

`softap` 显式选择 `CONFIG_ESP32_WIFI_SOFTAP`；`blewifi`、`sta_softap`、`wifishare` 选择 `CONFIG_ESP32_WIFI_STATION_SOFTAP`。其他配置未写模式时，Kconfig 默认 STA。出处：[softap:32](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/configs/softap/defconfig:32)、[sta_softap:33](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/configs/sta_softap/defconfig:33)、[E/Kconfig:2182](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2182)。

**esp32-2432S028 的显示接入完整，触摸接入未找到：**

- `lvgl` 配置启用 SPI2、`CONFIG_LCD_DEV`、`CONFIG_LCD_ILI9341`、`CONFIG_LCD_ILI9341_IFACE0` 和 `CONFIG_SPI_CMDDATA`。bringup 调用 `board_lcd_initialize()`、设置背光、`lcddev_register(0)`，形成 `/dev/lcd0`。出处：[defconfig:24](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-2432S028/configs/lvgl/defconfig:24)、[bringup:329](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-2432S028/src/esp32_bringup.c:329)。
- 公共实现为 `B/common/src/esp32_ili9341.c`，调用 `esp32_spibus_initialize()` 和 `ili9341_initialize()`。板级定义：SPI2、DC=GPIO2、RST=-1〔注释称接 EN〕、背光=GPIO21；未覆盖的 SPI2 默认引脚为 CS15、CLK14、MOSI13、MISO12。出处：[公共驱动:363](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_ili9341.c:363)、[board.h:46](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-2432S028/include/board.h:46)、[E/Kconfig:1358](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:1358)。
- **`CONFIG_LV_USE_NUTTX_TOUCHSCREEN=y` 只是 LVGL 输入适配选项。**该板配置没有选择硬件触摸驱动；在该板目录和 `B/common/` 未检出触摸控制器初始化、触摸 GPIO/总线配置或 `/dev/inputN` 注册。不能据此把该板称为“显示和触摸都已接通”的参考。出处：[defconfig:47](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-2432S028/configs/lvgl/defconfig:47)、[板级目录](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-2432S028)、[公共板级目录](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common)。
- 文档把 GPIO2 写为 LCD Reset，与上述源码的 DC=2 不一致。出处：[板级文档:65](/home/huiqin123/Git/nuttx/Documentation/platforms/xtensa/esp32/boards/esp32-2432S028/index.rst:65)。

**3．经典 ESP32 外设、Flash 与 WiFi**

下表 Kconfig 符号均带 `CONFIG_` 前缀。

| 功能 | 已有实现、接口与配置 | 出处 |
|---|---|---|
| SPI master | SPI2、SPI3；`esp32_spibus_initialize(port)`。`CONFIG_ESP32_SPI2/SPI3`，可选 `CONFIG_ESP32_SPI2_DMA/SPI3_DMA`；`CONFIG_SPI_DRIVER` 提供 `/dev/spiN` 注册包装 | [E/esp32_spi.c:1490](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_spi.c:1490)、[E/Kconfig:609](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:609)、[common/esp32_board_spidev.c:154](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_board_spidev.c:154) |
| I2C master | I2C0、I2C1；`esp32_i2cbus_initialize()`；`CONFIG_ESP32_I2C0/I2C1`。`CONFIG_I2C_DRIVER` 包装注册 `/dev/i2cN` | [E/esp32_i2c.c:1467](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_i2c.c:1467)、[E/Kconfig:866](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:866)、[common/esp32_board_i2c.c:52](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_board_i2c.c:52) |
| GPIO、中断 | `esp32_configgpio/gpioread/gpiowrite`；`CONFIG_ESP32_GPIO_IRQ`；底层有上升、下降、双边沿及高低电平中断类型 | [E/esp32_gpio.c:212](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_gpio.c:212)、[E/esp32_gpio.h:96](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_gpio.h:96)、[E/Kconfig:971](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:971) |
| `/dev/gpioN` | `CONFIG_DEV_GPIO`；DevKitC 的 `esp32_gpio_init()` 调 `gpio_pin_register()`。映射由板级代码决定，并非自动暴露全部 GPIO | [DevKitC/esp32_gpio.c:329](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/src/esp32_gpio.c:329)、[drivers/ioexpander/Kconfig:495](/home/huiqin123/Git/nuttx/drivers/ioexpander/Kconfig:495) |
| LEDC PWM | `esp32_ledc.c`、`esp32_ledc_init(timer)`；`CONFIG_ESP32_LEDC`、`CONFIG_ESP32_LEDC_TIM0…TIM3`、`CONFIG_ESP32_LEDC_CHANNEL0_PIN…CHANNEL7_PIN`；DevKitC 注册 `/dev/pwm0…3` | [E/esp32_ledc.c:770](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_ledc.c:770)、[E/Kconfig:2521](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2521)、[DevKitC/esp32_ledc.c:56](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/src/esp32_ledc.c:56) |
| timer | `esp32_tim.c`、`esp32_tim_lowerhalf.c`；`CONFIG_ESP32_TIMER0…3`＋`CONFIG_TIMER`；`esp32_timer_initialize()` 注册 `/dev/timerN` | [E/Kconfig:656](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:656)、[E/esp32_tim_lowerhalf.c:518](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_tim_lowerhalf.c:518) |
| one-shot | `esp32_oneshot.c`、`esp32_oneshot_lowerhalf.c`；`CONFIG_ESP32_ONESHOT`＋`CONFIG_ONESHOT`；公共包装注册 `/dev/oneshot` | [E/Kconfig:2430](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2430)、[common/esp32_oneshot.c:56](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_oneshot.c:56) |
| RTC | `esp32_rtc.c`、`esp32_rtc_lowerhalf.c`；`CONFIG_ESP32_RTC`、`CONFIG_RTC_DRIVER`，可选 `CONFIG_RTC_ALARM`；注册 `/dev/rtc0`，有读写时间及 alarm 接口 | [E/Kconfig:712](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:712)、[E/esp32_rtc_lowerhalf.c:102](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_rtc_lowerhalf.c:102)、[:533](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_rtc_lowerhalf.c:533) |

具体边界：

- DevKitC GPIO 示例映射为输出 GPIO15、输入 GPIO18、中断 GPIO22；该中断示例固定使用上升沿。出处：[板级 GPIO:57](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/src/esp32_gpio.c:57)、[:308](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/src/esp32_gpio.c:308)。
- LEDC 实现使用低速寄存器，提供 4 个 timer；多通道配置下最多 8 个通道，同一 timer 共用频率。出处：[E/esp32_ledc.c:49](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_ledc.c:49)、[:119](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_ledc.c:119)、[:650](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_ledc.c:650)。
- **WiFi 会选择 RT timer，并占用 Timer0。**DevKitC bringup 因此不再将 Timer0 注册为普通 timer；one-shot 使用 Timer1。出处：[E/Kconfig:737](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:737)、[E/esp32_rt_timer.c:62](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_rt_timer.c:62)、[DevKitC bringup:392](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/src/esp32_bringup.c:392)。
- RTC 默认内部 RC 时钟；源码不足以证明掉电后仍保留绝对时间。RTC 内存 magic 无效时会清零时间偏移。出处：[E/Kconfig:2450](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2450)、[E/esp32_rtc.c:2023](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_rtc.c:2023)。

**Flash 原始字库区域：已有指定范围 MTD 能力。**

- `CONFIG_ESP32_SPIFLASH` 选择 `CONFIG_MTD`、`CONFIG_MTD_BYTE_WRITE`、`CONFIG_MTD_PARTITION`，没有 PSRAM 依赖。范围选项是 `CONFIG_ESP32_STORAGE_MTD_OFFSET`、`CONFIG_ESP32_STORAGE_MTD_SIZE`。出处：[E/Kconfig:602](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:602)、[:1972](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:1972)。
- `esp32_spiflash_alloc_mtdpart(offset,size,encrypted)` 按字节偏移、长度创建分区；检查整片容量边界和实际 sector 对齐。公共板级代码使用上述两个 Kconfig 参数；不选文件系统时注册 `/dev/esp32flash`。出处：[E/esp32_spiflash.c:2622](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_spiflash.c:2622)、[common/esp32_board_spiflash.c:376](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_board_spiflash.c:376)、[:425](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_board_spiflash.c:425)。
- 按 2 MiB 计算，字库区为 **`[0x15D000,0x35D000)`**，落在 4 MiB 内；起点和长度均为 4 KiB 整数倍。驱动实际使用 ROM 提供的 `sector_size`，本次未读取硬件，因此只能确认：**若 sector 为 4096 字节，则满足其对齐条件**。该入口不检查与固件或其他分区是否重叠。出处：[E/esp32_spiflash.c:323](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_spiflash.c:323)、[:2652](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_spiflash.c:2652)。
- 裸 MTD 注册不自动等于无条件可 `open/read/lseek`。本树在 `CONFIG_BCH=y` 且未禁用 mountpoint、pseudofs 操作时，才对 MTD 的 `open()` 建立代理。底层 MTD 自身已有 `read/bread` 接口。出处：[N/fs/vfs/fs_open.c:184](/home/huiqin123/Git/nuttx/fs/vfs/fs_open.c:184)、[E/esp32_spiflash.c:312](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_spiflash.c:312)。
- 另有 `CONFIG_ESP32_PARTITION_TABLE`，位于 `CONFIG_ESP32_APP_FORMAT_LEGACY` 条件下，可解析分区表并注册 MTD；`CONFIG_ESP32_PARTITION_MOUNTPT` 默认 `/dev/esp/partition/`。出处：[E/Kconfig:2003](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2003)、[E/esp32_partition.c:747](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_partition.c:747)。

**WiFi 三种模式均有实现。**

`CONFIG_ESP32_WIFI` 下的模式是三选一：

```text
CONFIG_ESP32_WIFI_STATION
CONFIG_ESP32_WIFI_SOFTAP
CONFIG_ESP32_WIFI_STATION_SOFTAP
```

组合模式同时建立 STA、SoftAP 两个设备；另一侧已启动时，适配代码实际调用 `esp_wifi_set_mode(WIFI_MODE_APSTA)`。公共注册入口为 `board_wlan_init()`，底层是 `esp32_wlan.c`、`esp32_wifi_adapter.c`。出处：[E/Kconfig:2182](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2182)、[E/esp32_wifi_adapter.h:56](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_wifi_adapter.h:56)、[adapter.c:4701](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_wifi_adapter.c:4701)、[common/esp32_board_wlan.c:65](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_board_wlan.c:65)。

模式启停路径会停止并重新启动共享 WiFi 栈，不能据此宣称 STA/AP 可无扰动独立切换。出处：[adapter.c:4695](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_wifi_adapter.c:4695)、[:5938](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_wifi_adapter.c:5938)。

FreeRTOS 对应的基础能力也存在，但公开 API 不同：任务 `task_create()`、线程 `pthread_create()`、消息队列 `mq_*()`、信号量 `sem_*()`、软件定时器 `timer_*()`。出处：[N/include/sched.h:227](/home/huiqin123/Git/nuttx/include/sched.h:227)、[pthread.h:528](/home/huiqin123/Git/nuttx/include/pthread.h:528)、[mqueue.h:75](/home/huiqin123/Git/nuttx/include/mqueue.h:75)、[semaphore.h:174](/home/huiqin123/Git/nuttx/include/semaphore.h:174)、[time.h:226](/home/huiqin123/Git/nuttx/include/time.h:226)。

**4．SSD1681、LCD/framebuffer、FT6336 与 LVGL**

- **SSD1681 驱动已存在，文件名是 `ssd1680.c`。**`CONFIG_LCD_SSD1680` 的标题明确为 SSD1680/SSD1681；`CONFIG_LCD_SSD1681_1_54` 明确选择 **200×200 单色**。还有 SSD1680 的 122×250、128×296 单色／三色配置。没有独立 `ssd1681.c`。出处：[N/drivers/lcd/Kconfig:1660](/home/huiqin123/Git/nuttx/drivers/lcd/Kconfig:1660)、[:1688](/home/huiqin123/Git/nuttx/drivers/lcd/Kconfig:1688)、[驱动目录](/home/huiqin123/Git/nuttx/drivers/lcd)。
- SPI 选项为 `CONFIG_SSD1680_SPIMODE`〔默认 0〕、`CONFIG_SSD1680_FREQUENCY`〔默认 5 MHz〕，需要 `CONFIG_SPI_CMDDATA`。驱动接口为 `ssd1680_initialize(spi,priv)`；板级提供 reset、busy、可选供电回调。出处：[LCD Kconfig:1695](/home/huiqin123/Git/nuttx/drivers/lcd/Kconfig:1695)、[公共头:141](/home/huiqin123/Git/nuttx/include/nuttx/lcd/ssd1680.h:141)、[:185](/home/huiqin123/Git/nuttx/include/nuttx/lcd/ssd1680.h:185)。
- 驱动有 shadow framebuffer；`putrun()` 写内存，`redraw()` 执行屏幕更新。现有 1bpp 像素格式不等于项目原有位序/API 已经兼容。出处：[ssd1680.c:340](/home/huiqin123/Git/nuttx/drivers/lcd/ssd1680.c:340)、[:389](/home/huiqin123/Git/nuttx/drivers/lcd/ssd1680.c:389)、[公共头:103](/home/huiqin123/Git/nuttx/include/nuttx/lcd/ssd1680.h:103)。
- **ESP32 板级接入参考也已有。**`B/common/src/esp32_ssd1680.c` 完成 GPIO、SPI 初始化及驱动绑定；`ttgo_eink5_v2:fb` 使用它。该参考板是 128×296 屏，定义 DC19、RST12、BUSY4、CS5、SPI3，并非用户项目的现成引脚配置。出处：[公共接入:126](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/src/esp32_ssd1680.c:126)、[TTGO board.h:60](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/ttgo_eink5_v2/include/board.h:60)、[N/boards/Kconfig:340](/home/huiqin123/Git/nuttx/boards/Kconfig:340)。

LCD/framebuffer 框架的实际组成：

| 接口 | 配置、作用 | 出处 |
|---|---|---|
| LCD 下层 | `struct lcd_dev_s`、`lcd_planeinfo_s`；提供 `putrun/putarea/redraw` 等 | [include/nuttx/lcd/lcd.h:86](/home/huiqin123/Git/nuttx/include/nuttx/lcd/lcd.h:86)、[:196](/home/huiqin123/Git/nuttx/include/nuttx/lcd/lcd.h:196) |
| LCD 字符设备 | `CONFIG_LCD_DEV`；`lcddev_register()` 注册 `/dev/lcdN` | [drivers/lcd/Kconfig:27](/home/huiqin123/Git/nuttx/drivers/lcd/Kconfig:27)、[lcd_dev.c:415](/home/huiqin123/Git/nuttx/drivers/lcd/lcd_dev.c:415) |
| LCD 转 framebuffer | `CONFIG_LCD_FRAMEBUFFER`；`lcd_framebuffer.c` 将刷新转换为 LCD 写入及可选 redraw | [LCD Kconfig:35](/home/huiqin123/Git/nuttx/drivers/lcd/Kconfig:35)、[lcd_framebuffer.c:223](/home/huiqin123/Git/nuttx/drivers/lcd/lcd_framebuffer.c:223) |
| framebuffer 字符设备 | `CONFIG_VIDEO_FB`；实际文件是 **`drivers/video/fb.c`，没有 `drivers/video/fb/` 目录**；注册 `/dev/fbN` 等 | [video/Kconfig:49](/home/huiqin123/Git/nuttx/drivers/video/Kconfig:49)、[fb.c:1726](/home/huiqin123/Git/nuttx/drivers/video/fb.c:1726) |

触摸方面：

- 有 `drivers/input/ft5x06.c`，配置为 `CONFIG_INPUT_FT5X06`；另有 `CONFIG_INPUT_FT5336`，后者选择前者且依赖 `CONFIG_EXPERIMENTAL`。可选 `CONFIG_FT5X06_POLLMODE`、`CONFIG_FT5X06_SINGLEPOINT`、`CONFIG_FT5X06_SWAPXY`。未找到 FT6x36/FT6336 专用驱动或选项。出处：[input/Kconfig:188](/home/huiqin123/Git/nuttx/drivers/input/Kconfig:188)、[输入驱动目录](/home/huiqin123/Git/nuttx/drivers/input)。
- `ft5x06_register(i2c,config,minor)` 注册 `/dev/inputN`，地址、频率和中断等由板级配置提供。出处：[公共接口:80](/home/huiqin123/Git/nuttx/include/nuttx/input/ft5x06.h:80)、[:124](/home/huiqin123/Git/nuttx/include/nuttx/input/ft5x06.h:124)。
- 与 DesktopScreen 对照，模式寄存器 `0x00`、手势 `0x01`、触点数 `0x02`、触点起始 `0x03/0x09`、坐标高低字节解码相符。但 NuttX 按最多 5 点、从 `0x01` 连读 32 字节；项目只处理两点，并另设阈值、活动周期和中断模式。**FT6336 完整兼容性不确定。**出处：[N/ft5x06.h:48](/home/huiqin123/Git/nuttx/drivers/input/ft5x06.h:48)、[ft5x06.c:256](/home/huiqin123/Git/nuttx/drivers/input/ft5x06.c:256)、[项目 ds_ft6336.h:14](/home/huiqin123/esp/DesktopScreen/main/include/driver/ds_ft6336.h:14)、[ds_ft6336.c:30](/home/huiqin123/esp/DesktopScreen/main/src/driver/ds_ft6336.c:30)、[:152](/home/huiqin123/esp/DesktopScreen/main/src/driver/ds_ft6336.c:152)。

**本机 Vela apps 的 LVGL 实际源码版本为 9.1.0**，入口 `CONFIG_GRAPHICS_LVGL`。外层 Makefile 的后备下载版本为 9.2.1，不能代替本地源码版本。出处：[A/graphics/lvgl/Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/graphics/lvgl/Kconfig:6)、[实际 lvgl.h:16](/home/huiqin123/Git/vela-opensource/apps/graphics/lvgl/lvgl/lvgl.h:16)、[Makefile:102](/home/huiqin123/Git/vela-opensource/apps/graphics/lvgl/Makefile:102)。

**5．文件系统与 NVS 等价能力**

| 项目 | 本机事实 | 出处 |
|---|---|---|
| SPIFFS | `CONFIG_FS_SPIFFS`，源码在 `N/fs/spiffs/src/`；Kconfig 注明移植自 0.3.7 | [fs/spiffs/Kconfig:6](/home/huiqin123/Git/nuttx/fs/spiffs/Kconfig:6) |
| LittleFS | `CONFIG_FS_LITTLEFS`；已有 `lfs_vfs.c` 和构建接入；`CONFIG_FS_LITTLEFS_VERSION` 默认 v2.5.1。**N 中尚无 `fs/littlefs/littlefs/` 上游源码目录**，构建文件含下载配方 | [fs/littlefs/Kconfig:167](/home/huiqin123/Git/nuttx/fs/littlefs/Kconfig:167)、[Make.defs:35](/home/huiqin123/Git/nuttx/fs/littlefs/Make.defs:35)、[:62](/home/huiqin123/Git/nuttx/fs/littlefs/Make.defs:62) |
| FAT | `CONFIG_FS_FAT`，本树 FAT 实现位于 `N/fs/fat/`，不能直接等同第三方 FatFs | [fs/fat/Kconfig:6](/home/huiqin123/Git/nuttx/fs/fat/Kconfig:6)、[fs/fat 目录](/home/huiqin123/Git/nuttx/fs/fat) |
| ESP32 文件系统接入 | 公共板级已有 `CONFIG_ESP32_SPIFLASH_SPIFFS`、`CONFIG_ESP32_SPIFLASH_LITTLEFS`，另有 SMARTFS、NXFFS；这是 optional choice，不选可注册裸 MTD | [B/common/Kconfig:22](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/common/Kconfig:22) |
| 配置持久化 | `CONFIG_MTD_CONFIG` 提供 MTD 上的 `/dev/config`；`CONFIG_MTD_CONFIG_NAMED` 支持命名键；`CONFIG_MTD_CONFIG_FAIL_SAFE` 选择 fail-safe NVS 实现 | [drivers/mtd/Kconfig:145](/home/huiqin123/Git/nuttx/drivers/mtd/Kconfig:145)、[:176](/home/huiqin123/Git/nuttx/drivers/mtd/Kconfig:176)、[:189](/home/huiqin123/Git/nuttx/drivers/mtd/Kconfig:189) |

`configdata` 接口是 `CFGDIOC_GETCONFIG/SETCONFIG/DELCONFIG` 等 ioctl，注册入口 `mtdconfig_register()`；它不提供 ESP-IDF `nvs_open/nvs_set_*` 的相同 API。出处：[N/include/nuttx/mtd/configdata.h:63](/home/huiqin123/Git/nuttx/include/nuttx/mtd/configdata.h:63)、[:123](/home/huiqin123/Git/nuttx/include/nuttx/mtd/configdata.h:123)。

Vela 另有：

- `CONFIG_KVDB`，位于 `V/frameworks/system/utils/kvdb/`；后端为 `CONFIG_KVDB_UNQLITE`、`CONFIG_KVDB_NVS`、`CONFIG_KVDB_FILE`。出处：[frameworks/system/utils/Kconfig:51](/home/huiqin123/Git/vela-opensource/frameworks/system/utils/Kconfig:51)、[:139](/home/huiqin123/Git/vela-opensource/frameworks/system/utils/Kconfig:139)。
- KVDB 的“NVS”后端实际调用 `configdata` ioctl，默认设备 `/dev/config`；不是已经证实的 ESP-IDF NVS 兼容层。出处：[Kconfig:158](/home/huiqin123/Git/vela-opensource/frameworks/system/utils/Kconfig:158)、[kvdb/nvs.c:156](/home/huiqin123/Git/vela-opensource/frameworks/system/utils/kvdb/nvs.c:156)。
- UnQLite 实际源码在 `V/external/unqlite/unqlite/`，`CONFIG_UNQLITE`，源码版本 1.1.9。出处：[UnQLite Kconfig:17](/home/huiqin123/Git/vela-opensource/external/unqlite/Kconfig:17)、[unqlite.h:61](/home/huiqin123/Git/vela-opensource/external/unqlite/unqlite/unqlite.h:61)。

**ESP32 WiFi 内部的 NVS 不能充当项目 NVS。**`esp_nvs_open()` 等回调是 `DEBUGPANIC()`／失败返回的桩函数，初始化明确设置 `wifi_cfg.nvs_enable=0`。出处：[E/esp32_wifi_adapter.c:3039](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_wifi_adapter.c:3039)、[:4610](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/esp32_wifi_adapter.c:4610)。

**6．网络应用层**

本节 apps 均指 **A＝Vela apps**，不代表已确认与独立 N 兼容。

| 组件 | 路径及启用选项 | 已确认事实／出处 |
|---|---|---|
| HTTP client | `A/netutils/webclient/`；`CONFIG_NETUTILS_WEBCLIENT` | 依赖 TCP，已有 `webclient.c`。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/netutils/webclient/Kconfig:6) |
| Mbed TLS | `A/crypto/mbedtls/`；`CONFIG_CRYPTO_MBEDTLS` | 本机实际源码 **3.4.0**，不只是下载配方。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/crypto/mbedtls/Kconfig:6)、[build_info.h:39](/home/huiqin123/Git/vela-opensource/apps/crypto/mbedtls/mbedtls/include/mbedtls/build_info.h:39) |
| cJSON | `A/netutils/cjson/`；`CONFIG_NETUTILS_CJSON` | 实际源码 **1.7.15**；外层 `CONFIG_NETUTILS_CJSON_VERSION` 默认值为 1.7.12。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/netutils/cjson/Kconfig:6)、[cJSON.h:86](/home/huiqin123/Git/vela-opensource/apps/netutils/cjson/cJSON/cJSON.h:86) |
| NTP 校时 | `A/netutils/ntpclient/`；`CONFIG_NETUTILS_NTPCLIENT` | 依赖 UDP、socket options；有 `ntpc_start/stop`。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/netutils/ntpclient/Kconfig:6)、[ntpclient.c:1549](/home/huiqin123/Git/vela-opensource/apps/netutils/ntpclient/ntpclient.c:1549) |
| DHCP server | `A/netutils/dhcpd/`；`CONFIG_NETUTILS_DHCPD` | 已有服务器；`CONFIG_NETUTILS_DHCPD_DNSIP` 可配置向客户端发布的 DNS 地址。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/netutils/dhcpd/Kconfig:6)、[:69](/home/huiqin123/Git/vela-opensource/apps/netutils/dhcpd/Kconfig:69) |
| HTTP server | `A/netutils/webserver/`；`CONFIG_NETUTILS_WEBSERVER` | 已有 HTTP server，`CONFIG_NETUTILS_HTTPD_CGIPATH` 支持 URL→CGI 函数注册。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/netutils/webserver/Kconfig:6)、[:51](/home/huiqin123/Git/vela-opensource/apps/netutils/webserver/Kconfig:51) |
| thttpd | `A/netutils/thttpd/`；`CONFIG_NETUTILS_THTTPD` | 源码存在，Kconfig 依赖 `NXFLAT || FS_BINFS`。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/netutils/thttpd/Kconfig:6) |
| WAPI | **`A/wireless/wapi/`**；`CONFIG_WIRELESS_WAPI`、`CONFIG_WIRELESS_WAPI_CMDTOOL` | 不在 netutils；有 WiFi 配置库及 `wapi` 命令工具。[Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/wireless/wapi/Kconfig:6) |

**HTTPS 有接口，但 webclient 缺已接好的 TLS 后端。**

`webclient` 仅在 URL 为 HTTPS 且 `tls_ops != NULL` 时进入 TLS；否则返回 `-ENOTSUP`。头文件明确写“NULL means no https support”。本地 apps、frameworks、external 检索未找到为 `struct webclient_tls_ops` 实例化的 Mbed TLS 后端。因此，**只开启 `CONFIG_CRYPTO_MBEDTLS` 不会自动赋予 webclient HTTPS 能力**。出处：[webclient.c:1299](/home/huiqin123/Git/vela-opensource/apps/netutils/webclient/webclient.c:1299)、[webclient.h:240](/home/huiqin123/Git/vela-opensource/apps/include/netutils/webclient.h:240)、[:436](/home/huiqin123/Git/vela-opensource/apps/include/netutils/webclient.h:436)。

Vela 另有**已经接入 Mbed TLS 的 libcurl**：`V/external/curl/`，`CONFIG_LIB_CURL` 依赖 `CONFIG_LIB_ZLIB`、`CONFIG_CRYPTO_MBEDTLS`；配置头定义 `USE_MBEDTLS=1`，本机有 `curl/lib/vtls/mbedtls.c` 和 TLS handshake 调用。证书路径选项为 `CONFIG_LIB_CURL_CA_PATH`。这属于另一套现成 HTTPS 客户端实现，不能等同于 webclient 后端。出处：[curl/Kconfig:17](/home/huiqin123/Git/vela-opensource/external/curl/Kconfig:17)、[curl_config.h:757](/home/huiqin123/Git/vela-opensource/external/curl/curl_config.h:757)、[mbedtls.c:806](/home/huiqin123/Git/vela-opensource/external/curl/curl/lib/vtls/mbedtls.c:806)。

**DNS／captive portal：**

- 在当前 `A/netutils/`、`A/examples/` 及相关 Vela frameworks/external 检索范围，**未找到普通 UDP 53 captive DNS 服务器或完整配网 portal 示例**；没有 `captivedns` 组件。出处：[A/netutils/Kconfig](/home/huiqin123/Git/vela-opensource/apps/netutils/Kconfig)、[A/examples 目录](/home/huiqin123/Git/vela-opensource/apps/examples)。
- `CONFIG_NETDB_DNSCLIENT` 是解析客户端；`V/external/mdns` 是 mDNS/DNS-SD，使用 5353，均不能计为 captive DNS。出处：[N/libs/libc/netdb/Kconfig:80](/home/huiqin123/Git/nuttx/libs/libc/netdb/Kconfig:80)、[V/external/mdns/Kconfig:6](/home/huiqin123/Git/vela-opensource/external/mdns/Kconfig:6)、[README:31](/home/huiqin123/Git/vela-opensource/external/mdns/README.md:31)。
- DHCP server、HTTP server 的独立组件和示例存在；配网页、请求处理、DNS 劫持及配置保存的完整业务组合未检出。出处：[A/examples/dhcpd/Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/examples/dhcpd/Kconfig:6)、[A/examples/webserver/Kconfig:6](/home/huiqin123/Git/vela-opensource/apps/examples/webserver/Kconfig:6)。

**7．构建入口、bootloader 与本机工具**

N 的配置脚本语法为：

```text
tools/configure.sh -l -a <相对于 nuttx 的 apps 路径> <板名>:<配置名>
```

`-l` 选择 Linux；`-a` 指定 apps；`-L [板名]` 是列配置选项。例如板级选择字符串可为 `esp32-devkitc:nsh`、`esp32-devkitc:wifi`。出处：[N/tools/configure.sh:30](/home/huiqin123/Git/nuttx/tools/configure.sh:30)。

源码文档规定的构建流程是：配置板级 → 可选 `make menuconfig` → 按镜像格式准备 bootloader → `make` 构建；烧录入口为：

```text
make flash ESPTOOL_PORT=<串口> ESPTOOL_BINDIR=<bootloader和分区表目录>
```

此处仅列文档用法，全部未执行。出处：[ESP32 文档:102](/home/huiqin123/Git/nuttx/Documentation/platforms/xtensa/esp32/index.rst:102)、[:127](/home/huiqin123/Git/nuttx/Documentation/platforms/xtensa/esp32/index.rst:127)、[DevKitC 文档:160](/home/huiqin123/Git/nuttx/Documentation/platforms/xtensa/esp32/boards/esp32-devkitc/index.rst:160)。

| 镜像方式 | bootloader／分区处理 | 出处 |
|---|---|---|
| Simple Boot | 默认无 Legacy、MCUboot 时启用 `CONFIG_ESPRESSIF_SIMPLE_BOOT`；不需要外置二级 bootloader，应用烧录偏移 `0x1000` | [E/Kconfig:2776](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2776)、[tools/esp32/Config.mk:132](/home/huiqin123/Git/nuttx/tools/esp32/Config.mk:132) |
| Legacy ESP-IDF 格式 | `CONFIG_ESP32_APP_FORMAT_LEGACY`；`make bootloader` 配方生成 `bootloader-esp32.bin`、`partition-table-esp32.bin`。bootloader 偏移 `0x1000`，分区表默认 `0x8000`，应用 `0x10000` | [E/Bootloader.mk:174](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Bootloader.mk:174)、[Config.mk:73](/home/huiqin123/Git/nuttx/tools/esp32/Config.mk:73)、[E/Kconfig:2831](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2831) |
| MCUboot | `CONFIG_ESP32_APP_FORMAT_MCUBOOT`；独立 bootloader、主／次 OTA slot 配置 | [E/Kconfig:2788](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:2788)、[Config.mk:105](/home/huiqin123/Git/nuttx/tools/esp32/Config.mk:105) |

**本机工具检查结果：**

| 工具 | 当前 PATH／系统 Python | 本机其他位置 |
|---|---|---|
| `xtensa-esp32-elf-gcc` | PATH 未找到 | **存在**于 `~/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/`，同目录也有 `xtensa-esp-elf-gcc` |
| `kconfig-mconf/conf/tweak` | PATH 均未找到 | **存在**于 `V/prebuilts/build-tools/linux-x86_64/bin/` |
| `menuconfig/olddefconfig/savedefconfig` | PATH 未找到；`/usr/bin/python3` 找不到 `kconfiglib` | **存在**于 `V/prebuilts/tools/python/bin/`；库在 `V/prebuilts/tools/python/dist-packages/kconfiglib/` |
| `esptool.py` | PATH 未找到；系统 Python 找不到模块 | **存在**于 `~/.espressif/python_env/idf5.4_py3.12_env/bin/`，该环境安装 **esptool 4.10.0** |
| ESP-IDF Kconfig Python 包 | 系统 Python 未找到 | 上述 IDF 环境存在 `esp-idf-kconfig 2.5.0`；不能仅据此视为 NuttX 所需命令已进入 PATH |
| 基础工具 | `make`、`gcc/g++`、`cmake`、`ninja`、`python3`、`pip`、`gperf`、`bison`、`flex`、`genromfs`、`xxd` 均可找到 | 路径为 `/usr/bin/…` |

出处为上述目录的 `ls`、`shutil.which()`、Python `find_spec()` 只读检查；版本另见 [esptool METADATA:3](/home/huiqin123/.espressif/python_env/idf5.4_py3.12_env/lib/python3.12/site-packages/esptool-4.10.0.dist-info/METADATA:3)、[esp-idf-kconfig METADATA:3](/home/huiqin123/.espressif/python_env/idf5.4_py3.12_env/lib/python3.12/site-packages/esp_idf_kconfig-2.5.0.dist-info/METADATA:3)。

相关构建事实：

- N 使用 `CONFIG_XTENSA_TOOLCHAIN_ESP` 时，编译器前缀就是 `xtensa-esp32-elf-`。本地文档示例工具链为 GCC 12.2.0，现有目录为 14.2.0；**未编译，兼容性不确定**。出处：[lx6/Toolchain.defs:42](/home/huiqin123/Git/nuttx/arch/xtensa/src/lx6/Toolchain.defs:42)、[ESP32 文档:49](/home/huiqin123/Git/nuttx/Documentation/platforms/xtensa/esp32/index.rst:49)。
- N 的 Make 系统优先使用 PATH 中的 `menuconfig`／kconfiglib 命令，否则使用 `kconfig-conf/mconf`。出处：[tools/Unix.mk:674](/home/huiqin123/Git/nuttx/tools/Unix.mk:674)。
- 文档写 esptool `4.8.dev4`；实际构建检查最低版本为 **4.8.0**，用于 ELF 转 ESP32 镜像和烧录。已安装的 4.10.0 高于此数值要求。出处：[ESP32 文档:130](/home/huiqin123/Git/nuttx/Documentation/platforms/xtensa/esp32/index.rst:130)、[Config.mk:71](/home/huiqin123/Git/nuttx/tools/esp32/Config.mk:71)。
- **N 的 ESP32 HAL 依赖尚未展开。**`E/esp-hal-3rdparty/` 不存在；Make 配方指定提交 `87ccbf88…`，无线配置还需其 WiFi/PHY/BT/coex 等子模块。当前 N 根目录也没有 Legacy bootloader、分区表二进制。出处：[E/Make.defs:215](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Make.defs:215)、[:266](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Make.defs:266)，对应路径只读检查。
- DevKitC 的 `nsh` 默认选择 `CONFIG_ARCH_CHIP_ESP32WROVER`；源码另有选择 4 MB Flash 的 `CONFIG_ARCH_CHIP_ESP32WROOM32`，而 `CONFIG_ESP32_SPIRAM` 是独立开关、默认关闭。因此不能把现成 `nsh` 的模块型号直接视作用户板型号。出处：[nsh/defconfig:17](/home/huiqin123/Git/nuttx/boards/xtensa/esp32/esp32-devkitc/configs/nsh/defconfig:17)、[E/Kconfig:40](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:40)、[:625](/home/huiqin123/Git/nuttx/arch/xtensa/src/esp32/Kconfig:625)。

**8．sim：支持 X11 显示和鼠标触摸模拟**

**可以用于宿主机 UI 调试，源码与预设均存在；本次没有运行验证。**

- `CONFIG_SIM_X11FB` 提供 X11 图形／输入模拟；图形接口选择 `CONFIG_SIM_LCDDRIVER` 或 `CONFIG_SIM_FRAMEBUFFER`；`CONFIG_SIM_TOUCHSCREEN` 用 X11 鼠标模拟触摸，依赖 `CONFIG_INPUT`。出处：[N/arch/sim/Kconfig:418](/home/huiqin123/Git/nuttx/arch/sim/Kconfig:418)、[:480](/home/huiqin123/Git/nuttx/arch/sim/Kconfig:480)、[:501](/home/huiqin123/Git/nuttx/arch/sim/Kconfig:501)。
- 实现文件为 `arch/sim/src/sim/sim_lcd.c`、`sim_framebuffer.c`、`sim_touchscreen.c`、`posix/sim_x11framebuffer.c`、`posix/sim_x11eventloop.c`。触摸注册为 `/dev/inputN`，最大点数为 1。出处：[sim_touchscreen.c:99](/home/huiqin123/Git/nuttx/arch/sim/src/sim/sim_touchscreen.c:99)、[sim_x11eventloop.c:131](/home/huiqin123/Git/nuttx/arch/sim/src/sim/posix/sim_x11eventloop.c:131)。
- `boards/sim/sim/sim/configs/` 有 `lvgl_fb`、`lvgl_lcd`、`fb`、`touchscreen`、`nx`、`nx11`、`nxlines`、`nxwm` 等配置。`lvgl_fb`、`lvgl_lcd` 均配置 640×480、X11、模拟触摸和 LVGL widgets 示例。出处：[sim 配置目录](/home/huiqin123/Git/nuttx/boards/sim/sim/sim/configs)、[lvgl_fb/defconfig:24](/home/huiqin123/Git/nuttx/boards/sim/sim/sim/configs/lvgl_fb/defconfig:24)、[lvgl_lcd/defconfig](/home/huiqin123/Git/nuttx/boards/sim/sim/sim/configs/lvgl_lcd/defconfig)。
- 本地文档明确给出 `sim:lvgl_fb` 和运行 `./nuttx` 显示 LVGL 窗口的例子。它模拟 LCD/framebuffer 与鼠标输入，不能据此认定 SSD1681 刷新时序或 FT6336 I2C 协议受到模拟。出处：[sim 文档:61](/home/huiqin123/Git/nuttx/Documentation/platforms/sim/sim/index.rst:61)。
- 本机已有 `/usr/include/X11/Xlib.h`、`/usr/lib/x86_64-linux-gnu/libX11.so`、`libXext.so`；环境变量 `DISPLAY=:0`。这些只证明文件和环境变量存在，未验证 X server 连接。出处：上述路径及只读环境检查。

末表状态沿用前文范围；标为“缺失”的项目指本次本地检索范围内未找到。

| 功能 | NuttX 现成方案（路径／Kconfig） | 状态 |
|---|---|---|
| SSD1681 200×200、1bpp | `N/drivers/lcd/ssd1680.c`；`CONFIG_LCD_SSD1680`＋`CONFIG_LCD_SSD1681_1_54`；已有 `B/common/src/esp32_ssd1680.c`，目标板引脚接入未现成提供 | 需适配 |
| LCD／framebuffer 接口 | `N/drivers/lcd/lcd_dev.c`、`lcd_framebuffer.c`、`drivers/video/fb.c`；`CONFIG_LCD_DEV/LCD_FRAMEBUFFER/VIDEO_FB` | 现成 |
| FT6336 | `N/drivers/input/ft5x06.c`、`CONFIG_INPUT_FT5X06` 可作为已有代码；完整 FT6336 兼容性不确定 | 需适配 |
| SPI master | `E/esp32_spi.c`；`CONFIG_ESP32_SPI2/SPI3` | 现成 |
| I2C master | `E/esp32_i2c.c`；`CONFIG_ESP32_I2C0/I2C1` | 现成 |
| GPIO／GPIO 中断 | `E/esp32_gpio.c`；`CONFIG_ESP32_GPIO_IRQ` | 现成 |
| 项目 `/dev/gpioN` 引脚映射 | DevKitC `src/esp32_gpio.c`；`CONFIG_DEV_GPIO`，现有示例管脚与目标板需对应 | 需适配 |
| LEDC PWM | `E/esp32_ledc.c`；`CONFIG_ESP32_LEDC`、`CONFIG_ESP32_LEDC_TIM0…3` | 现成 |
| timer／one-shot／RTC | `E/esp32_tim*`、`esp32_oneshot*`、`esp32_rtc*`；对应 `CONFIG_TIMER/ONESHOT/RTC_DRIVER`；WiFi 占 Timer0 | 现成 |
| 原始 Flash 字库读取 | `esp32_spiflash_alloc_mtdpart()`；`CONFIG_ESP32_STORAGE_MTD_OFFSET/SIZE`；目标范围与文件访问接口需配置 | 需适配 |
| SPIFFS | `N/fs/spiffs/`；`CONFIG_FS_SPIFFS`、`CONFIG_ESP32_SPIFLASH_SPIFFS` | 现成 |
| LittleFS | `N/fs/littlefs/`；`CONFIG_FS_LITTLEFS`；已有接入，上游源码依赖尚未展开 | 现成 |
| FAT | `N/fs/fat/`；`CONFIG_FS_FAT` | 现成 |
| NVS 类配置持久化 | `N/drivers/mtd/mtd_config*.c`；`CONFIG_MTD_CONFIG/NAMED/FAIL_SAFE`；API 与 ESP-IDF 不同 | 需适配 |
| ESP-IDF `nvs_*` 兼容层 | 当前 ESP32 WiFi NVS 为禁用的桩；未检出可用兼容层 | 缺失 |
| Vela KVDB／UnQLite | `V/frameworks/system/utils/kvdb/`、`V/external/unqlite/`；`CONFIG_KVDB`、`CONFIG_UNQLITE` | 现成 |
| WiFi STA／SoftAP／STA+AP | `E/esp32_wlan.c`、`esp32_wifi_adapter.c`；`CONFIG_ESP32_WIFI`＋三种模式选项；HAL/无线依赖未展开 | 现成 |
| WiFi 配置工具 | `A/wireless/wapi/`；`CONFIG_WIRELESS_WAPI/CMDTOOL` | 现成 |
| HTTP 客户端 | `A/netutils/webclient/`；`CONFIG_NETUTILS_WEBCLIENT` | 现成 |
| webclient HTTPS | `webclient_tls_ops`＋`A/crypto/mbedtls/`；没有找到已实现的连接回调层 | 需适配 |
| Vela libcurl HTTPS | `V/external/curl/`；`CONFIG_LIB_CURL`＋`CONFIG_CRYPTO_MBEDTLS`，已有 Mbed TLS 后端 | 现成 |
| cJSON | `A/netutils/cjson/`；`CONFIG_NETUTILS_CJSON`，本地源码 1.7.15 | 现成 |
| SNTP／NTP 校时功能 | `A/netutils/ntpclient/`；`CONFIG_NETUTILS_NTPCLIENT` | 现成 |
| DHCP server | `A/netutils/dhcpd/`；`CONFIG_NETUTILS_DHCPD` | 现成 |
| HTTP server | `A/netutils/webserver/` 或 `thttpd/`；`CONFIG_NETUTILS_WEBSERVER/THTTPD` | 现成 |
| 普通 captive DNS server | 当前 apps／相关 Vela 组件未检出；DNS resolver、mDNS 不等价 | 缺失 |
| 完整 captive portal 配网业务 | 有 AP、DHCP、HTTP 组件；未检出完整业务组合，且缺上述 DNS 服务端 | 需适配 |
| FreeRTOS 任务／队列／信号量调用 | `task_create/pthread_*`、`mq_*`、`sem_*` 已有；项目调用 API 不同 | 需适配 |
| LVGL | `A/graphics/lvgl/`；`CONFIG_GRAPHICS_LVGL`，实际源码 9.1.0 | 现成 |
| PC UI／触摸模拟 | `N/boards/sim/sim/sim/configs/lvgl_fb`、`lvgl_lcd`；`CONFIG_SIM_X11FB/SIM_TOUCHSCREEN` | 现成 |
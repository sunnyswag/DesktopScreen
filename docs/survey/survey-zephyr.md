> Zephyr v4.5.0（浅克隆在 ~/Git/zephyrproject/zephyr，未 west update，模块源码不在本地）。由 Codex 于 2026-10-31 摸底，关键结论已抽查核实。原项目行号基于旧提交 e18432a。

以下路径均以 **`Z=/home/huiqin123/Git/zephyrproject/zephyr`** 为根。版本文件为 **4.5.0**。本次仅查本地源码、绑定、Kconfig、文档及已有调查报告；未修改文件，未执行 git、west、CMake、编译、烧录或联网。外部模块的内部实现未核验。[VERSION:1](/home/huiqin123/Git/zephyrproject/zephyr/VERSION:1)

**1．ESP32 板级、默认外设、分区、模块声明**

完整板名为：

- `esp32_devkitc/esp32/procpu`：core 0。
- `esp32_devkitc/esp32/appcpu`：core 1 的独立镜像目标，使用 `slot0_appcpu_partition`；不是通用的“双核开关”。
- 另有 `esp32_devkitc/esp32/procpu/qemu`。

出处：[PROCPU 描述:1](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc_procpu.yaml:1)、[APPCPU DTS:15](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc_appcpu.dts:15)、[QEMU 描述:4](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc_procpu_qemu.yaml:4)、[Kconfig.soc:16](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/esp32/Kconfig.soc:16)。

PROCPU 的 DTS 默认状态：

| 外设 | 默认状态及配置 | 出处，相对 Z |
|---|---|---|
| UART0 | 启用，115200，console/shell | `boards/espressif/esp32_devkitc/esp32_devkitc_procpu.dts:35`、`:54` |
| GPIO0、GPIO1 | 启用，另列保留引脚范围 | 同文件`:73` |
| I2C0 | 启用，100kHz，SDA21/SCL22 | 同文件`:95` |
| SPI2、SPI3 | 启用，有默认 pinctrl | 同文件`:114` |
| TRNG、BT HCI、WiFi | DTS 启用 | 同文件`:146` |
| WDT0、SHA、AES | SoC DTS 启用 | `dts/xtensa/espressif/esp32/esp32_common.dtsi:400`、`:566` |
| Flash、pinctrl、clock、interrupt controller | 基础节点启用；部分通过省略 `status` 启用 | 同文件`:79`、`:185`、`:203` |
| UART1/2、I2S0/1、内置 touch | 板级虽设置部分属性，仍继承 `disabled` | 板 DTS`:61`、`:83`、`:104`；SoC DTS`:243`、`:335`、`:365` |
| I2C1、LEDC、MCPWM、timer0～3、RTC timer 等 | 默认未启用 | SoC DTS`:194`、`:270`、`:354`；板 DTS`:130` |

**DTS 启用不等于驱动已经编译。** PROCPU defconfig 仅显式打开 `CONSOLE/SERIAL/UART_CONSOLE/GPIO`；还需要对应子系统和驱动 Kconfig。出处：[defconfig:4](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc_procpu_defconfig:4)、[DTS troubleshooting:57](/home/huiqin123/Git/zephyrproject/zephyr/doc/build/dts/troubleshooting.rst:57)。

Flash 默认 **4MiB**。DTS 虽写 `psram0.size=8MiB`，默认 defconfig 没有启用 `CONFIG_ESP_SPIRAM`，不能把这个声明当作 WROOM-32 实际有 PSRAM。另有 `CONFIG_ESP32_USE_UNSUPPORTED_REVISION`，帮助文字把经典 ESP32 Rev0/Rev1 列为当前实现的非正常支持范围；本次未确定用户芯片修订号。[板 DTS:45](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc_procpu.dts:45)、[Kconfig.spiram:9](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/common/Kconfig.spiram:9)、[common/Kconfig:13](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/common/Kconfig:13)。

分区 include 链为：

`esp32_devkitc_procpu.dts:12` → `dts/vendor/espressif/partitions_0x1000_amp.dtsi:7` → `partitions_0x1000_amp_4M.dtsi`。

当前表位于 `&flash0 { partitions { ... }; };`，**每个分区的 compatible 是 `zephyr,mapped-partition`**，不是该板采用了旧的 `fixed-partitions` 父节点结构。

| 分区标签 | 偏移 | 长度 | 分区文件行号 |
|---|---:|---:|---:|
| `boot_partition` | `0x1000` | 60KiB | 13 |
| `sys_partition` | `0x10000` | 64KiB | 19 |
| `slot0_partition` | `0x20000` | 1344KiB | 25 |
| `slot1_partition` | `0x170000` | 1344KiB | 31 |
| `slot0_appcpu_partition` | `0x2c0000` | 448KiB | 37 |
| `slot1_appcpu_partition` | `0x330000` | 448KiB | 43 |
| `slot0_lpcore_partition` | `0x3a0000` | 32KiB | 49 |
| `slot1_lpcore_partition` | `0x3a8000` | 32KiB | 55 |
| `storage_partition` | `0x3b0000` | **192KiB** | 61 |
| `coredump_partition` | `0x3ff000` | 4KiB | 67 |

出处：[完整默认分区文件](/home/huiqin123/Git/zephyrproject/zephyr/dts/vendor/espressif/partitions_0x1000_amp_4M.dtsi:7)。其中 LP-core 名称来自共享模板，不代表经典 ESP32 有 LP core。按 `reg` 计算，storage 结束于 `0x3e0000`，到 coredump 前还有 124KiB 空隙；文件末尾“Remaining flash size is 0kB”注释不能代替这些实际数值。

Overlay 可以通过 `&storage_partition { reg = <offset size>; };` 覆盖已有分区；整体替换已有官方实例：先在 `&flash0` 内 `/delete-node/ partitions;`，再包含另一份分区 DTS。`reg` 的两个值是相对父 Flash 的偏移和字节长度。`DTC_OVERLAY_FILE` 可指定文件；`EXTRA_DTC_OVERLAY_FILE` 在其后追加；`app.overlay` 是自动查找的兜底项，并非总会与板级 overlay 合并。[分区绑定:37](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/mtd/zephyr,mapped-partition.yaml:37)、[官方替换实例:7](/home/huiqin123/Git/zephyrproject/zephyr/snippets/espressif/flash-4M/soc/flash_0x1000_amp_4M.overlay:7)、[overlay 查找规则:225](/home/huiqin123/Git/zephyrproject/zephyr/doc/build/dts/howtos.rst:225)。

`west.yml` 声明如下；路径相对 **west 工作区根目录**。这些模块均不在本地，以下依据 manifest：

| 模块 | 声明路径 | 固定 revision | 出处 |
|---|---|---|---|
| hal_espressif | `modules/hal/espressif` | `cc81db00233af6020f2fea9fbb0d0098fb3d0d86` | [west.yml:174](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:174) |
| LVGL | `modules/lib/gui/lvgl` | `c9ffd4692de246eb911c7ba49b78a331b2c63c74` | [west.yml:324](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:324) |
| LittleFS | `modules/fs/littlefs` | `e9a8638fc228e43a3156ed3ac602229af3e3c1d5` | [west.yml:313](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:313) |
| FatFs | `modules/fs/fatfs` | `4f60a12c58c3de4246857ad87c40239ecd93349b` | [west.yml:144](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:144) |
| Mbed TLS | `modules/crypto/mbedtls` | `c43f34b93797e81fd0257c30004dcd0ae332ae51` | [west.yml:327](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:327) |
| MCUboot | `bootloader/mcuboot` | `725d63c9d5bc1bb572d802e7a768daf504186daf` | [west.yml:332](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:332) |

**2．ESP32 外设驱动**

下表 Kconfig 均带 `CONFIG_` 前缀；除驱动选项外，还需开启对应 `SPI/I2C/GPIO/PWM/COUNTER/FLASH` 子系统。

| 功能 | compatible | 驱动与 Kconfig 出处 |
|---|---|---|
| SPI master | `espressif,esp32-spi` | `drivers/spi/spi_esp32_spim.c`；`ESP32_SPIM`，`drivers/spi/Kconfig.esp32:6` |
| I2C master | `espressif,esp32-i2c` | `drivers/i2c/i2c_esp32.c`；`I2C_ESP32`，`drivers/i2c/Kconfig.esp32:7` |
| GPIO、中断 | `espressif,esp32-gpio` | `drivers/gpio/gpio_esp32.c`；`GPIO_ESP32`，`drivers/gpio/Kconfig.esp32:6` |
| pinctrl | `espressif,esp32-pinctrl` | `drivers/pinctrl/pinctrl_esp32.c`；`PINCTRL_ESP32`，`drivers/pinctrl/Kconfig.esp32:4` |
| LEDC PWM | `espressif,esp32-ledc` | `drivers/pwm/pwm_led_esp32.c`；`PWM_LED_ESP32`，`drivers/pwm/Kconfig.esp32:7` |
| MCPWM | `espressif,esp32-mcpwm` | `drivers/pwm/pwm_mc_esp32.c`；`MCPWM_ESP32`，同上`:15` |
| GP timer / counter | 父节点 `espressif,esp32-timer`，子节点 `espressif,esp32-counter` | `drivers/counter/counter_esp32_tmr.c`；`COUNTER_TMR_ESP32`，`drivers/counter/Kconfig.esp32_tmr:6` |
| RTC counter | `espressif,esp32-rtc-timer` | `drivers/counter/counter_esp32_rtc.c`；`COUNTER_RTC_ESP32`，`drivers/counter/Kconfig.esp32_rtc:6` |
| 内置 SPI Flash | `espressif,esp32-flash-controller`，子节点 `soc-nv-flash` | `drivers/flash/flash_esp32.c`；`SOC_FLASH_ESP32`，`drivers/flash/Kconfig.esp32:4` |

对应绑定分别在 `dts/bindings/{spi,i2c,gpio,pinctrl,pwm,counter,flash_controller}/` 下同名 YAML。

SPI 确实使用 GPIO matrix：pinctrl 调用 `esp_rom_gpio_matrix_out/in()`。它支持在具备对应输入/输出能力的 GPIO 上路由，不能理解为任何编号均无限制可用；绑定也明确说明引脚能力限制。`use-iomux` 是特定引脚绕过 matrix 的可选方式。GPIO 软件片选由总线 `cs-gpios` 提供，子设备 `reg` 是数组索引。[pinctrl 实现:253](/home/huiqin123/Git/zephyrproject/zephyr/drivers/pinctrl/pinctrl_esp32.c:253)、[引脚限制:61](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/pinctrl/espressif,esp32-pinctrl.yaml:61)、[SPI 绑定:4](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/spi/espressif,esp32-spi.yaml:4)。

项目 SPI 引脚对应的宏在主仓中全部存在，以下是按绑定语法组成的片段，未写入文件：

```dts
&pinctrl {
	screen_spi: screen_spi {
		group1 {
			pinmux = <SPIM2_SCLK_GPIO25>,
				 <SPIM2_MOSI_GPIO26>,
				 <SPIM2_MISO_GPIO23>;
		};
	};
};

&spi2 {
	status = "okay";
	pinctrl-0 = <&screen_spi>;
	pinctrl-names = "default";
	cs-gpios = <&gpio0 27 GPIO_ACTIVE_LOW>;
};
```

宏出处：[esp32-pinctrl.h:4643](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/dt-bindings/pinctrl/esp32-pinctrl.h:4643)、同文件`:4686`、`:4720`；片选写法见上述 SPI 绑定`:25`。板默认 SPI2 是 12/14/15/13，SPI3 包含 GPIO5、23，与项目配置不同。[板 pinctrl:44](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc-pinctrl.dtsi:44)

I2C1 已有 `I2C1_SDA_GPIO33`、`I2C1_SCL_GPIO32`；pinctrl group 支持 `bias-pull-up; drive-open-drain; output-high;`，总线频率用 `clock-frequency=<I2C_BITRATE_STANDARD>`。classic ESP32 的 master 支持存在，但 `I2C_TARGET` 模式被其 Kconfig 帮助文字明确排除。[宏:171](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/dt-bindings/pinctrl/esp32-pinctrl.h:171)、同文件`:207`、[pinctrl 示例:87](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/pinctrl/espressif,esp32-pinctrl.yaml:87)、[I2C Kconfig:15](/home/huiqin123/Git/zephyrproject/zephyr/drivers/i2c/Kconfig.esp32:15)。

GPIO 驱动有上升沿、下降沿、双沿、高电平、低电平中断映射，回调通过 `gpio_init_callback()` / `gpio_add_callback()` 接入。因此 GPIO4 双沿能力在底层存在；FT5336 驱动自身选用的边沿见第 4 项。[gpio_esp32.c:425](/home/huiqin123/Git/zephyrproject/zephyr/drivers/gpio/gpio_esp32.c:425)、同文件`:496`、`:534`。

LEDC 和 MCPWM 都已实现 Zephyr PWM API。LEDC 的通道、GPIO、timer 由 pinctrl 和子节点 `reg/timer` 配置；有 `LEDC_CH0_GPIO15`。`pwm_set()` 使用纳秒周期/脉宽，4kHz 对应周期 250000ns，50% 对应 125000ns。MCPWM 绑定明确经典 ESP32 有两组，通道 2 对应 timer1/operator1/PWM1A。[LEDC 绑定:52](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/pwm/espressif,esp32-ledc.yaml:52)、[GPIO15 宏:709](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/dt-bindings/pinctrl/esp32-pinctrl.h:709)、[pwm.h:660](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/drivers/pwm.h:660)、[MCPWM 绑定:11](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/pwm/espressif,esp32-mcpwm.yaml:11)。

GP counter 实现 start/stop、alarm、top-value、64-bit 接口；RTC 驱动是持续运行的计数器/闹钟，不能直接等同日历 RTC。经典 ESP32 的系统 tick 驱动是基于 CCOUNT/CCOMPARE 的 `CONFIG_XTENSA_TIMER`。[counter API 表:736](/home/huiqin123/Git/zephyrproject/zephyr/drivers/counter/counter_esp32_tmr.c:736)、[RTC counter:104](/home/huiqin123/Git/zephyrproject/zephyr/drivers/counter/counter_esp32_rtc.c:104)、[Kconfig.xtensa:7](/home/huiqin123/Git/zephyrproject/zephyr/drivers/timer/Kconfig.xtensa:7)。

Flash 驱动提供 read/write/erase/page-layout；正常读取最终调用 HAL 的 `esp_flash_read()`，因此底层 HAL 行为仍受模块缺失这一核验边界限制。[flash_esp32.c:141](/home/huiqin123/Git/zephyrproject/zephyr/drivers/flash/flash_esp32.c:141)、同文件`:768`。

**3．SSD1681、局部刷新及像素格式**

**SSD1681 明确受支持。** 绑定 `solomon,ssd1681` 写明 200×200，驱动为 `drivers/display/ssd16xx.c`，配置为 `CONFIG_DISPLAY`、`CONFIG_SSD16XX`，后者选择 `MIPI_DBI`。[绑定:4](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/display/solomon,ssd1681.yaml:4)、[Kconfig:6](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/Kconfig.ssd16xx:6)

现成参考是 **`waveshare_epaper_gdeh0154a07`** shield。其 overlay 使用：

- 父节点 `zephyr,mipi-dbi-spi`，在父节点配置 `spi-dev/dc-gpios/reset-gpios`。
- 子节点 compatible 为 `"gooddisplay,gdeh0154a07", "solomon,ssd1681"`。
- `width=200`、`height=200`、SPI 4MHz、BUSY active-high、`tssv=0x80`。
- `full { border-waveform=<0x05>; };`
- `partial { border-waveform=<0x3c>; };`

该示例采用 Arduino 引脚，并非项目 GPIO25/26/27/14/12/13 的现成配置。[shield overlay:15](/home/huiqin123/Git/zephyrproject/zephyr/boards/shields/waveshare_epaper/waveshare_epaper_gdeh0154a07.overlay:15)

**局刷实现存在，其触发条件不是写入矩形的大小。**

| 调用/配置 | SSD16xx 实际行为 | 出处 |
|---|---|---|
| 没有 `partial` 子节点 | 不启用 partial profile | `dts/bindings/display/solomon,ssd16xx-common.yaml:53` |
| 有 `partial`，且 blanking=false | 每次 `display_write()` 切入 partial profile，写窗口并刷新；即使写满整屏也如此 | `drivers/display/ssd16xx.c:432`、`:444`、`:466` |
| `display_blanking_on()` | 从未 blanking 状态进入时，切 FULL profile，随后暂缓刷新 | 同文件`:301` |
| blanking=true 时 `display_write()` | 写 RAM，不刷新；另维护局刷所需旧画面 | 同文件`:466`、`:473` |
| 随后的 `display_blanking_off()` | 刷新当前 FULL profile；原本已是 false 时不额外刷新 | 同文件`:289` |

出处：[刷新路径:425](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:425)、[blanking 实现:289](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:289)。

`full/partial` 子节点都可带 `lut` 字节数组。没有数组时使用控制器 OTP；空 `partial {}` 也能表示使用 OTP 默认值。驱动在 profile 改变时 soft reset 并装载配置，相同 profile 直接返回。上述 shield 配置下，由代码位值计算，FULL 的 `0x22` 参数为 **`0xF7`**，PARTIAL 为 **`0xFF`**，随后发 `0x20`。[profile 绑定:46](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/display/solomon,ssd16xx-common.yaml:46)、[LUT 加载:731](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:731)、[刷新命令:268](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:268)、[SSD1681 参数:1037](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:1037)、[寄存器位值:68](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx_regs.h:68)。

**没有“五次局刷后全刷”的自动计数或阈值配置。** 驱动状态和 write 路径只依据 blanking/profile；该频率策略属于应用逻辑。原调查报告记录的原项目判断实际为 `partial_update_time>5`。[驱动状态:75](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:75)、[原报告:216](/home/huiqin123/esp/DesktopScreen/docs/survey-desktopscreen.md:216)。

像素与缓冲约束：

- 只接受 **`PIXEL_FORMAT_MONO01`，0=黑、1=白**；MONO10 返回 `-ENOTSUP`。
- MSB first；NORMAL/180° 时为 **VTILED：每字节垂直八像素**，不是项目原有横向 25 字节/行。
- `pitch` 必须等于 `width`；NORMAL/180° 的 y、height 必须八像素对齐；90°/270° 的 x、width 必须八像素对齐。
- 200×200×1bpp 仍为 5000 字节，但相同字节数不代表布局兼容。

出处：[capabilities/pixel format:570](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:570)、[区域约束:330](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:330)、[格式定义:77](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/drivers/display.h:77)、[原项目布局:236](/home/huiqin123/esp/DesktopScreen/docs/survey-desktopscreen.md:236)。

**该驱动初始化不要求额外 `blanking_off()` 才启动显示。** 初始化已设 `blanking_on=false`、清 RAM 并执行一次 FULL update。它也没有实现原项目每刷后 `0x10 0x01` 的屏幕 deep sleep 流程，没有挂接 device PM；blanking_on 不是屏幕深睡。SSD16xx 未提供 `display_get_framebuffer()` 指针操作，应用缓冲通过 `display_write()` 提交。[初始化:898](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:898)、[API 表:982](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:982)、[设备注册:1133](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:1133)、[framebuffer API:679](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/drivers/display.h:679)。

**4．FT6336 与 Input 子系统**

现成驱动是 `drivers/input/input_ft5336.c`，compatible **`focaltech,ft5336`**，配置 **`CONFIG_INPUT_FT5336`**。Kconfig 列出的家族包括 FT5x06、FT5606、FT5x16、FT6x06、Ft6x36、FT5x06i、FT5336、FT3316、FT5436i、FT3267、FT5336i、FT5x46；其中没有逐字列 `FT6336`。[Kconfig.ft5336:5](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/Kconfig.ft5336:5)

但树内有 **明确 FT6336 板级实例**：

- Arduino Nesso N1 的 `ft6336@38` 使用 `focaltech,ft5336`、地址 `0x38`。
- M5Stack Core2 文档列 FT6336U，其 DTS 同样使用此 compatible。

因此不能因驱动文件名是 FT5336 就判定 FT6336 缺失。[Nesso DTS:150](/home/huiqin123/Git/zephyrproject/zephyr/boards/arduino/nesso_n1/arduino_nesso_n1_esp32c6_hpcore.dts:150)、[Core2 文档:17](/home/huiqin123/Git/zephyrproject/zephyr/boards/m5stack/m5stack_core2/doc/index.rst:17)、[Core2 DTS:195](/home/huiqin123/Git/zephyrproject/zephyr/boards/m5stack/m5stack_core2/m5stack_core2_procpu.dts:195)。

中断模式由 `CONFIG_INPUT_FT5336_INTERRUPT` 控制，有 `int-gpios` 时默认开启。驱动固定配置 **`GPIO_INT_EDGE_TO_ACTIVE`**，active-low INT 对应下降沿，**不是项目双沿模式**；ISR 只提交 work，由 work 做 I2C 读取。关闭中断模式后，已有 timer→work 轮询，`INPUT_FT5336_PERIOD_MS` 默认 **10ms**。[中断配置:272](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/input_ft5336.c:272)、[轮询配置:19](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/Kconfig.ft5336:19)。Nesso N1 还因该板 IRQ 不可靠而默认选择轮询，此说明只针对该板。[板 Kconfig:11](/home/huiqin123/Git/zephyrproject/zephyr/boards/arduino/nesso_n1/Kconfig.defconfig:11)

驱动上报坐标和 `INPUT_BTN_TOUCH` 按下/释放事件；默认最多一点，可配置 1～5 点。应用入口是：

```c
void touch_cb(struct input_event *evt, void *user_data);
INPUT_CALLBACK_DEFINE(touch_device_or_NULL, touch_cb, user_data);
```

NULL 接收全部设备事件；事件包含 `dev/sync/type/code/value`。Input 默认通过专用线程分发，也可配置同步方式，不要求使用 LVGL。[上报:105](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/input_ft5336.c:105)、[input.h:47](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/input/input.h:47)、同文件`:159`、[Input Kconfig:21](/home/huiqin123/Git/zephyrproject/zephyr/subsys/input/Kconfig:21)。

绑定支持 `reset-gpios`、`int-gpios`，公共触摸属性支持屏幕尺寸、轴交换、反转及输出坐标缩放。现项目引脚和 200→152 坐标关系不是现成板配置。[FT 绑定](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/input/focaltech,ft5336.yaml)、[公共触摸绑定:9](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/input/touchscreen-common.yaml:9)。

**5．Settings、文件系统、裸 Flash 字库**

Settings 的两种后端配置组合为：

| 后端 | 配置项 | 出处，相对 Z |
|---|---|---|
| NVS | `CONFIG_SETTINGS`、`CONFIG_SETTINGS_NVS`、`CONFIG_NVS`、`CONFIG_FLASH_MAP`；NVS 依赖 `FLASH/FLASH_PAGE_LAYOUT` | `subsys/settings/Kconfig:115`；`subsys/kvss/nvs/Kconfig:6` |
| ZMS | `CONFIG_SETTINGS`、`CONFIG_SETTINGS_ZMS`、`CONFIG_ZMS`、`CONFIG_FLASH_MAP` | `subsys/settings/Kconfig:69`；`subsys/kvss/zms/Kconfig:8` |

应用已有 `settings_subsys_init()`、`settings_save_one(name,value,len)`、`settings_load_one()`、`settings_load()`、`settings_delete()`，以及静态/动态 handler 注册。两个后端优先使用 chosen `zephyr,settings-partition`，否则取 `storage_partition`。[Settings API:248](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/settings/settings.h:248)、同文件`:311`、`:387`、[NVS 分区选择:20](/home/huiqin123/Git/zephyrproject/zephyr/subsys/settings/src/settings_nvs.c:20)、[ZMS 分区选择:21](/home/huiqin123/Git/zephyrproject/zephyr/subsys/settings/src/settings_zms.c:21)。

这能承载项目字符串键值，但不是 ESP-IDF `nvs_open/nvs_set_*/nvs_commit` 兼容 API。直接 Zephyr NVS 使用数值 ID；当前正式头为 `<zephyr/kvss/nvs.h>`、`<zephyr/kvss/zms.h>`，旧 `fs/nvs.h`、`fs/zms.h` 已 deprecated。主仓未检出 IDF NVS 兼容层；**hal_espressif 不在本地，其内部是否提供相关接口不确定**。[nvs.h:92](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/kvss/nvs.h:92)、[旧头:11](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/fs/nvs.h:11)、[west.yml:174](/home/huiqin123/Git/zephyrproject/zephyr/west.yml:174)。

文件系统情况：

- **LittleFS**：`CONFIG_FILE_SYSTEM`＋`CONFIG_FILE_SYSTEM_LITTLEFS`；主仓适配在 `subsys/fs/littlefs_fs.c`，明确依赖外部 LittleFS 模块，Flash 路径使用 flash_area。
- **FAT**：`CONFIG_FILE_SYSTEM`＋`CONFIG_FAT_FILESYSTEM_ELM`，选择 `DISK_ACCESS`；主仓适配在 `subsys/fs/fat_fs.c`，链接外部 ELM FatFs。
- **SPIFFS**：在主仓文件系统实现、Kconfig、manifest 中未检出。文件上传功能本身不能据此算作缺少 HTTP server，但原 SPIFFS 格式/API 不现成存在。
- Vela KVDB、UnQLite 在此主仓和 manifest 中也未检出。

出处：[LittleFS Kconfig:6](/home/huiqin123/Git/zephyrproject/zephyr/subsys/fs/Kconfig.littlefs:6)、[LittleFS Flash 接入:175](/home/huiqin123/Git/zephyrproject/zephyr/subsys/fs/littlefs_fs.c:175)、[FAT Kconfig:6](/home/huiqin123/Git/zephyrproject/zephyr/subsys/fs/Kconfig.fatfs:6)、[文件系统构建表:10](/home/huiqin123/Git/zephyrproject/zephyr/subsys/fs/CMakeLists.txt:10)、[west.yml](/home/huiqin123/Git/zephyrproject/zephyr/west.yml)。

**字库可直接按分区偏移读取，不需要文件系统。** 当前板的自定义分区写法是在 `flash0` 下的 `partitions` 分组中定义节点标签，例如 `font_partition`，设置 `compatible="zephyr,mapped-partition"`、`label="font"`、`reg=<OFFSET SIZE>`；OFFSET、SIZE 分别是 Flash 内偏移和字节数。父分组具有 `ranges; #address-cells=<1>; #size-cells=<1>;`。[mapped-partition 绑定:37](/home/huiqin123/Git/zephyrproject/zephyr/dts/bindings/mtd/zephyr,mapped-partition.yaml:37)、同文件`:73`。

对应 API 为：

```c
flash_area_open(PARTITION_ID(font_partition), &fa);
flash_area_read(fa, offset, dst, len);
flash_area_close(fa);
```

`offset` 相对分区起点。实现先检查边界，再调用 `flash_read(fa->fa_dev, fa->fa_off + offset, ...)`；当前宏是 `PARTITION_ID()`，`FIXED_PARTITION_ID()` 已 deprecated。默认分区表没有 1.48MB 字库区；默认 storage 仅192KiB。[flash_map.h:131](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/storage/flash_map.h:131)、同文件`:393`、[实际读取:51](/home/huiqin123/Git/zephyrproject/zephyr/subsys/storage/flash_map/flash_map.c:51)、[默认 storage:61](/home/huiqin123/Git/zephyrproject/zephyr/dts/vendor/espressif/partitions_0x1000_amp_4M.dtsi:61)。

**6．网络**

**STA＋SoftAP：现成支持。** `CONFIG_WIFI_ESP32` 对应 `espressif,esp32-wifi`，要求 RF blobs，且明确 `depends on !SMP`。并发模式是 `CONFIG_ESP32_WIFI_AP_STA_MODE`，依赖 `WIFI_NM`，在 `WIFI_USAGE_MODE_STA_AP` 下默认开启。驱动创建第二个 net_if，并在连接/启动 AP 时切换至 APSTA。[WiFi Kconfig:3](/home/huiqin123/Git/zephyrproject/zephyr/drivers/wifi/esp32/Kconfig.esp32:3)、同文件`:60`、[模式切换:1592](/home/huiqin123/Git/zephyrproject/zephyr/drivers/wifi/esp32/src/esp_wifi_drv.c:1592)、同文件`:1963`、`:2761`。

已有 `samples/net/wifi/apsta_mode`：通过 `net_if_get_wifi_sta()`、`net_if_get_wifi_sap()` 取得接口，以 `net_mgmt(NET_REQUEST_WIFI_CONNECT, ...)`、`NET_REQUEST_WIFI_AP_ENABLE` 启动。配置包含 `WIFI_NM`、`WIFI_USAGE_MODE_STA_AP`、两个 IPv4 接口及 DHCP client/server。WiFi 命令行工具是 `CONFIG_NET_L2_WIFI_SHELL`，实现 `subsys/net/l2/wifi/wifi_shell.c`，示例 `samples/net/wifi/shell/`。[APSTA sample:149](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/wifi/apsta_mode/src/main.c:149)、[sample 配置:2](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/wifi/apsta_mode/prj.conf:2)、[shell 配置:74](/home/huiqin123/Git/zephyrproject/zephyr/subsys/net/l2/Kconfig:74)。

**DHCPv4 server：现成主仓实现。** `CONFIG_NET_DHCPV4_SERVER`，实现 `subsys/net/lib/dhcpv4/dhcpv4_server.c`；API 为 `net_dhcpv4_server_start(iface, base_addr)` / `stop()`，地址池数量默认4。上述 APSTA 示例已经设置 AP 地址/掩码并启动它。[Kconfig:155](/home/huiqin123/Git/zephyrproject/zephyr/subsys/net/lib/dhcpv4/Kconfig:155)、[API:68](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/net/dhcpv4_server.h:68)、[sample:101](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/wifi/apsta_mode/src/main.c:101)。

**HTTP server：有现成实现，当前 Kconfig 标为 EXPERIMENTAL。** `CONFIG_HTTP_SERVER`，HTTP/1 默认开启；实现和公共头分别在 `subsys/net/lib/http/`、`include/zephyr/net/http/`。[Kconfig:52](/home/huiqin123/Git/zephyrproject/zephyr/subsys/net/lib/http/Kconfig:52)

`GET /` 和动态 POST 的现成写法见 `samples/net/sockets/http_server/`：

- `HTTP_SERVICE_DEFINE()` 定义监听服务。
- `http_resource_detail_static` 指定 GET、`text/html`、内嵌数组及长度；`HTTP_RESOURCE_DEFINE(..., "/", &detail)` 注册首页。sample 的网页/JS 编译成 gzip 字节数组，不要求文件系统。
- `http_resource_detail_dynamic` 指定 `BIT(HTTP_POST)` 和 `.cb`；callback 通过 `request_ctx->data/data_len` 接收分片，在 `HTTP_SERVER_REQUEST_DATA_FINAL` 时完成解析。
- `/led` 示例已演示拼接 JSON 请求体及 `json_obj_parse()`；其32字节缓冲只是该示例的限制。
- `http_server_start()` 启动服务。项目四个 POST 业务 handler 并未由 sample 提供。

出处：[静态资源:45](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/http_server/src/main.c:45)、[POST callback:182](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/http_server/src/main.c:182)、[服务和路由:259](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/http_server/src/main.c:259)、同文件`:386`。

**HTTP client：现成支持普通 HTTP GET。** `CONFIG_HTTP_CLIENT`，API 为 `http_client_req(sock, &req, timeout_ms, user_data)`；socket 必须由调用者先完成连接，request 提供 method/url/host/response callback 等。响应 body 可能分片到达。示例为 `samples/net/sockets/http_client/`。[client API:345](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/net/http/client.h:345)、[GET sample:191](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/http_client/src/main.c:191)。

用于与 NuttX 末表比较：同一 HTTP client 示例也有 **TLS socket→`http_client_req()`** 的完整 HTTPS 接法，包括 CA 注册、`IPPROTO_TLS_1_2`、`TLS_SEC_TAG_LIST`、`TLS_HOSTNAME`。Mbed TLS 模块不在本地；libcurl 未在主仓或 manifest 中检出。[TLS sample:59](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/http_client/src/main.c:59)、同文件`:145`、[TLS 配置:5](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/http_client/overlay-tls.conf:5)。

**SNTP：查询与设置系统时间分开。** `CONFIG_SNTP` 提供 `sntp_simple()`、`sntp_init/query/close()` 和异步接口。sample 查询并打印时间；它不会因此自动设置系统 wall clock。POSIX `clock_settime(CLOCK_REALTIME, ...)` 的实现另调用 `sys_clock_settime()`。[sntp.h:32](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/net/sntp.h:32)、同文件`:156`、[sample:92](/home/huiqin123/Git/zephyrproject/zephyr/samples/net/sockets/sntp_client/src/main.c:92)、[clock.c:56](/home/huiqin123/Git/zephyrproject/zephyr/subsys/portability/posix/options/clock.c:56)。

**不能无条件照搬 `TZ=CST-8; tzset()`。** Minimal libc 的 `localtime_r()` 直接调用 `gmtime_r()`，明确只返回 UTC。Picolibc/Newlib 的时区行为依赖所选外部库及配置；本地缺少相应实现，未确认本目标配置的 CST-8 行为。[minimal time.h:46](/home/huiqin123/Git/zephyrproject/zephyr/lib/libc/minimal/include/time.h:46)、[实现:11](/home/huiqin123/Git/zephyrproject/zephyr/lib/libc/common/source/time/localtime_r_utc.c:11)、[libc 选择:78](/home/huiqin123/Git/zephyrproject/zephyr/lib/libc/Kconfig:78)。

**JSON：主仓有描述符式库。** `CONFIG_JSON_LIBRARY`，实现 `lib/utils/json.c`；头 `<zephyr/data/json.h>` 提供 `JSON_OBJ_DESCR_PRIM/OBJECT/ARRAY`、`json_obj_parse()`、`json_obj_encode_buf()`。cJSON 没有作为本 manifest 的现成模块；外部 Mender 文档提到 cJSON 依赖，不等于主树已有 cJSON 实现。[Kconfig:6](/home/huiqin123/Git/zephyrproject/zephyr/lib/utils/Kconfig:6)、[json.h:180](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/data/json.h:180)、同文件`:670`、`:852`、[Mender 文档:26](/home/huiqin123/Git/zephyrproject/zephyr/doc/develop/manifest/external/mender-mcu.rst:26)。

**普通 captive DNS server：本地未检出。** `subsys/net/lib/dns/` 提供 resolver、DNS-SD、cache、mDNS responder、LLMNR responder；没有检出监听 UDP/53 并把任意域名回应为 AP 地址的现成实现/sample。mDNS 是5353端口，不等价。DHCP 的 DNS 地址选项和 captive-portal option114 也不等于 DNS 服务器。[DNS 构建表:6](/home/huiqin123/Git/zephyrproject/zephyr/subsys/net/lib/dns/CMakeLists.txt:6)、[mDNS:55](/home/huiqin123/Git/zephyrproject/zephyr/subsys/net/lib/dns/mdns_responder.c:55)、[DHCP 选项:229](/home/huiqin123/Git/zephyrproject/zephyr/subsys/net/lib/dhcpv4/Kconfig:229)。

**7．LVGL、CFB**

LVGL 的 manifest 固定提交见第1项；本树迁移文档写明 **从 v9.5 更新至 v9.6**。由于模块源码不在本地，这个版本结论依据文档，未读取模块版本宏。入口为 `CONFIG_LVGL`；主仓接入层在 `modules/lvgl/`，外部核心源码路径是工作区的 `modules/lib/gui/lvgl/`。[迁移文档:3249](/home/huiqin123/Git/zephyrproject/zephyr/doc/releases/migration-guide-4.5.rst:3249)、[接入 CMake:10](/home/huiqin123/Git/zephyrproject/zephyr/modules/lvgl/CMakeLists.txt:10)。

单色接入层能转换 MONO01/MONO10、VTILED/MSB 布局并做八像素对齐。但 **`CONFIG_LV_Z_PARTIAL_REFRESH` 指 LVGL 渲染方式，不能等同 SSD1681 局刷波形**：在非 DIRECT_RENDERING 模式，一轮 EPD flush 如果分成多块，非最后一块先调用 `display_blanking_on()`，最后一块再 `blanking_off()`；结合 SSD16xx 实现，这条路径会切 FULL profile。单块 flush 不必进入此分支。[mono 接入:88](/home/huiqin123/Git/zephyrproject/zephyr/modules/lvgl/lvgl_display_mono.c:88)、[LVGL render mode:65](/home/huiqin123/Git/zephyrproject/zephyr/modules/lvgl/lvgl.c:65)。

CFB 是主仓独立字符 framebuffer，配置 `CONFIG_CHARACTER_FRAMEBUFFER`，实现 `subsys/fb/`，无需 LVGL。提供 `cfb_framebuffer_init()`、`cfb_print()`、绘图和 `cfb_framebuffer_finalize()`；200×200 单色缓冲分配5000字节。finalize 提交一次全屏 `display_write()`，没有脏矩形裁剪、自动 blanking 或局刷计数。因此在 partial profile 已启用、blanking=false 的 SSD16xx 上，CFB 全屏提交仍会使用 partial profile。[CFB Kconfig:6](/home/huiqin123/Git/zephyrproject/zephyr/subsys/fb/Kconfig:6)、[API:93](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/display/cfb.h:93)、[finalize:706](/home/huiqin123/Git/zephyrproject/zephyr/subsys/fb/cfb.c:706)、同文件`:807`。

**8．ESP32 light sleep 与唤醒**

经典 ESP32 选择 `HAS_PM/HAS_POWEROFF`，`CONFIG_PM` 会编译 `soc/espressif/common/power.c`。CPU0 DTS 的 `light_sleep` 映射到 **`PM_STATE_STANDBY`**，默认最小驻留1000µs；deep sleep 映射 `soft-off`，默认禁用自动选择。[SoC Kconfig:15](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/esp32/Kconfig:15)、[CMake:25](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/esp32/CMakeLists.txt:25)、[power-states:30](/home/huiqin123/Git/zephyrproject/zephyr/dts/xtensa/espressif/esp32/esp32_common.dtsi:30)。

**RTC 定时唤醒路径明确存在。** STANDBY 入口先检查 RTC counter 可用及 `wakeup-source` 能力，配置 `esp_sleep_enable_timer_wakeup()`，再调用 `esp_light_sleep_start()`；缺 RTC 或可用睡眠时间不足时跳过。已有经典 ESP32 示例 `samples/boards/espressif/light_sleep/`，配置 `PM/PM_DEVICE/COUNTER`，overlay 启用 `rtc_timer` 并加 `wakeup-source`。[power.c:96](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/common/power.c:96)、同文件`:232`、[sample 配置](/home/huiqin123/Git/zephyrproject/zephyr/samples/boards/espressif/light_sleep/prj.conf)、[sample overlay:13](/home/huiqin123/Git/zephyrproject/zephyr/samples/boards/espressif/light_sleep/app.overlay:13)。

这里是 scheduler idle 驱动的睡眠，预算取内核下一超时及更早的 `esp_timer` alarm。因此60秒超时可作为唤醒期限，但仍活跃的10ms定时器会提供更早的期限，不能据 `k_sleep(60s)` 就宣称连续 light sleep 60秒。[power.c:317](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/common/power.c:317)、[PM Kconfig:22](/home/huiqin123/Git/zephyrproject/zephyr/subsys/pm/Kconfig:22)。

**GPIO 唤醒存在接入代码，但目标组合不能据本地材料认定已验证。**

- GPIO 配置处理 `GPIO_INPUT | GPIO_INT_WAKEUP`，按 `GPIO_ACTIVE_LOW` 选择低电平极性，在支持的 SoC 上调用 `esp_sleep_enable_ext1_wakeup_io()`；普通双沿 IRQ 与这个唤醒配置是两条逻辑。
- 经典 ESP32 deep-sleep 示例明确使用 GPIO4＋`GPIO_INT_WAKEUP`。
- light-sleep 示例 README 却仍写“当前仅支持 RTC timer 唤醒”。
- HAL 的 wakeup 实现/能力宏不在本地，且没有运行目标板。因此可确认主仓已有 GPIO/EXT1 接入，**GPIO4低电平＋60秒定时的目标 light-sleep 组合仍属未确认**，不能简单写成 GPIO 唤醒完全缺失或已经可直接使用。

出处：[GPIO 唤醒:256](/home/huiqin123/Git/zephyrproject/zephyr/drivers/gpio/gpio_esp32.c:256)、同文件`:463`、[GPIO4 示例:16](/home/huiqin123/Git/zephyrproject/zephyr/samples/boards/espressif/deep_sleep/boards/esp32_devkitc_procpu.overlay:16)、[light-sleep 文档:13](/home/huiqin123/Git/zephyrproject/zephyr/samples/boards/espressif/light_sleep/README.rst:13)。

触摸设备自身也有 PM 行为：FT5336 默认 suspend 使用 hibernate，要求硬件 reset 才恢复；`CONFIG_INPUT_FT5336_PM_MODE_MONITOR` 改用25fps monitor，并注明触摸可自动唤醒。这与原项目靠触摸 INT 唤醒相关，不能忽略设备自身的休眠状态。[触摸 PM Kconfig:50](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/Kconfig.ft5336:50)、[PM 实现:301](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/input_ft5336.c:301)。

WiFi 也有 PM hook，部分条件会返回 `-EBUSY` 阻止睡眠；系统 PM 会中止该次设备挂起。因而 APSTA 功能存在不能进一步推导为“APSTA持续工作时一定能按期进入60秒 light sleep”。[WiFi PM:2301](/home/huiqin123/Git/zephyrproject/zephyr/drivers/wifi/esp32/src/esp_wifi_drv.c:2301)、[系统 PM:49](/home/huiqin123/Git/zephyrproject/zephyr/subsys/pm/device_system_managed.c:49)。

**9．构建、SDK、烧录、ESP-IDF 工具链**

本地 Getting Started 的标准顺序是：Python venv → 安装 west → `west init` → `west update` → `west packages pip --install` → `west zephyr-export` → 在 Zephyr 目录执行 `west sdk install`。文档主机最低依赖为 CMake3.28.0、Python3.12、dtc1.4.6。[Getting Started:57](/home/huiqin123/Git/zephyrproject/zephyr/doc/develop/getting_started/index.rst:57)、同文件`:223`、`:298`、`:373`、`:409`、`:428`。

已有 clone 对应的官方初始化形式是：

```text
# 在 /home/huiqin123/Git/zephyrproject 中
west init -l zephyr
west update
west packages pip --install
west zephyr-export

# 在 zephyr 目录中
west sdk install
west blobs fetch hal_espressif
```

这只是文档命令说明，全部未执行。`west init -l` 会把现有仓库作为 manifest repository；Espressif 文档要求取得 RF blobs，并建议在 update 后 fetch。[west init 文档:62](/home/huiqin123/Git/zephyrproject/zephyr/doc/develop/west/built-in.rst:62)、[Espressif blobs:8](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/common/system-requirements.rst:8)。

**SDK 版本有两个不同数值：**本树 `SDK_VERSION` 是 **1.0.1**，供默认安装使用；CMake 查找 SDK 的最低版本参数是 **1.0**。SoC 的工具链名为 `espressif_esp32`；本地文档明确列出 SDK 内 `gnu/xtensa-espressif_esp32_zephyr-elf/bin/...`，完整 triplet 带 `_zephyr`。[SDK_VERSION:1](/home/huiqin123/Git/zephyrproject/zephyr/SDK_VERSION:1)、[FindHostTools:46](/home/huiqin123/Git/zephyrproject/zephyr/cmake/modules/FindHostTools.cmake:46)、[SDK 安装器:228](/home/huiqin123/Git/zephyrproject/zephyr/scripts/west_commands/sdk.py:228)、[SoC 工具链名:35](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/esp32/Kconfig.soc:35)、[SDK 路径实例:325](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/common/qemu.rst:325)。

用户现有 ESP-IDF `xtensa-esp-elf` GCC14.2 的结论是：

- **本树没有内建 `ZEPHYR_TOOLCHAIN_VARIANT=espressif`。** `cmake/toolchain/` 无 `espressif/`，构建按 variant 查找对应 `generic.cmake`。
- 通用 `ZEPHYR_TOOLCHAIN_VARIANT=cross-compile`＋`CROSS_COMPILE=<路径>/xtensa-esp-elf-` 的接入机制仍存在。
- 官方明确外部交叉编译器可能未经测试、不属于其正式支持；两个 ESP32 板描述只列 `zephyr` 工具链。**未编译，无法确认这套 IDF GCC14.2 可构建、链接并运行目标配置。**

出处：[工具链目录](/home/huiqin123/Git/zephyrproject/zephyr/cmake/toolchain)、[variant 加载:112](/home/huiqin123/Git/zephyrproject/zephyr/cmake/modules/FindHostTools.cmake:112)、[外部交叉编译器文档:10](/home/huiqin123/Git/zephyrproject/zephyr/doc/develop/toolchains/other_x_compilers.rst:10)、同文件`:32`、[板描述:5](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/esp32_devkitc_procpu.yaml:5)。

普通 PROCPU build 默认 **Simple Boot**，单个镜像，无需二级 bootloader，**MCUboot和sysbuild均非必需**。选择 `--sysbuild` 后，本板默认选择 MCUboot。Simple Boot 不提供 OTA/secure boot/slot管理。[启动文档:5](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/common/building-flashing.rst:5)、[ESP_SIMPLE_BOOT:21](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/common/Kconfig:21)、[sysbuild 默认:4](/home/huiqin123/Git/zephyrproject/zephyr/boards/espressif/esp32_devkitc/Kconfig.sysbuild:4)。

`west flash` 的 **runner名称是 `esp32`，内部工具是 `esptool`**。Simple Boot 烧写偏移取 `boot_partition`，即默认 `0x1000`；不能只看 chosen 的 slot0 就写成 `0x20000`。当前构建脚本提示需要 `esptool>=5.0.2`。[runner选择:3](/home/huiqin123/Git/zephyrproject/zephyr/boards/common/esp32.board.cmake:3)、[esptool调用:156](/home/huiqin123/Git/zephyrproject/zephyr/scripts/west_commands/runners/esp32.py:156)、[烧写偏移:91](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/common/CMakeLists.txt:91)、同文件`:4`。

此外，Simple Boot 链接 Flash 长度来源是物理 Flash 大小，不能把默认1344KiB slot声明当成应用自动避开数据分区的证据。[memory.h:129](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/esp32/memory.h:129)、[链接脚本:82](/home/huiqin123/Git/zephyrproject/zephyr/soc/espressif/esp32/default.ld:82)。

**10．native_sim：PC 显示、触摸、网络**

有现成的 SDL 显示与输入路径：

- `CONFIG_DISPLAY`＋`CONFIG_SDL_DISPLAY`，compatible `zephyr,sdl-dc`。
- `CONFIG_INPUT`＋`CONFIG_INPUT_SDL_TOUCH`，compatible `zephyr,input-sdl-touch`。
- 默认 DTS 是320×240 ARGB8888；尺寸、像素格式可通过 DTS 配置，SDL支持MONO01/MONO10。
- 鼠标左键按下/释放及移动转换为 `INPUT_ABS_X/Y`、`INPUT_BTN_TOUCH`，与应用接收的触摸事件体系一致。

出处：[native_sim DTS:213](/home/huiqin123/Git/zephyrproject/zephyr/boards/native/native_sim/native_sim.dts:213)、[SDL Kconfig:7](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/Kconfig.sdl:7)、[输入 Kconfig:4](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/Kconfig.sdl:4)、[鼠标输入:49](/home/huiqin123/Git/zephyrproject/zephyr/drivers/input/input_sdl_touch_bottom.c:49)、[SDL 像素格式:1273](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/display_sdl.c:1273)。

网络有两条主路径：

| 路径 | 覆盖范围与主机条件 | 出处 |
|---|---|---|
| `CONFIG_ETH_NATIVE_TAP` | 用主机 TAP `zeth`，运行 Zephyr 网络栈；需先配置虚拟接口 | `drivers/ethernet/Kconfig.native_tap:6`；`boards/native/native_sim/doc/index.rst:412` |
| `CONFIG_NET_NATIVE_OFFLOADED_SOCKETS` | 直接用宿主 BSD sockets，无需额外虚拟网络配置/root；不覆盖 Zephyr L2 | `drivers/net/Kconfig:252`；上述文档`:491` |

主机为 Linux 环境。默认 `native_sim` 是32位，需要32位 libc/multilib及32位SDL2开发库；`native_sim/native/64` 使用64位库。SDL构建要求 `pkg-config` 和对应位数的SDL2，Ubuntu文档列 `libsdl2-dev:i386`。[32/64位目标:151](/home/huiqin123/Git/zephyrproject/zephyr/boards/native/native_sim/doc/index.rst:151)、[SDL依赖:536](/home/huiqin123/Git/zephyrproject/zephyr/boards/native/native_sim/doc/index.rst:536)、[实际依赖查找:5](/home/huiqin123/Git/zephyrproject/zephyr/boards/native/common/sdl/CMakeLists.txt:5)。

这些可用于显示、输入和网络应用逻辑的 PC 调试；**不模拟 SSD1681 波形/残影/BUSY时序、FT6336 I2C/INT、电源唤醒或实际功耗**。SDL capabilities 也没有 `SCREEN_INFO_EPD`，默认不会进入第7项 LVGL 的 EPD blanking 分支。[native_sim范围:14](/home/huiqin123/Git/zephyrproject/zephyr/boards/native/native_sim/doc/index.rst:14)、[SDL screen_info:1281](/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/display_sdl.c:1281)。

**11．树外应用、自定义板、任务与定时**

官方支持两种树外应用：

- **workspace application**：位于 west 工作区内、Zephyr仓库外，例如 `zephyrproject/app/`。
- **freestanding application**：位于整个 west 工作区外；仍使用已安装/检出的 Zephyr 和模块。默认可由 CMake package registry 找到 Zephyr，也能以 `ZEPHYR_BASE` 指定。

出处：[应用组织:154](/home/huiqin123/Git/zephyrproject/zephyr/doc/develop/application/index.rst:154)、同文件`:176`、[CMake package查找:108](/home/huiqin123/Git/zephyrproject/zephyr/doc/build/zephyr_cmake_package.rst:108)。

自定义板可放应用或专门仓库的 `boards/<vendor>/<board>/`。`BOARD_ROOT` 指向**包含 `boards/` 的目录**；可以通过 `-DBOARD_ROOT=...` 传入，或在应用 `find_package(Zephyr ...)` 前 `list(APPEND BOARD_ROOT ...)`。CMakeLists 中须使用绝对路径；sysbuild时应在module或sysbuild自己的CMakeLists里设置。[树外板文档:1112](/home/huiqin123/Git/zephyrproject/zephyr/doc/develop/application/index.rst:1112)、同文件`:1175`、`:1190`、`:1203`、[BOARD_ROOT检查:167](/home/huiqin123/Git/zephyrproject/zephyr/cmake/modules/boards.cmake:167)。

项目使用的内核机制均有对应原生接口：线程 `k_thread_create/K_THREAD_DEFINE`，定长消息队列 `k_msgq_put/get`，信号量 `k_sem_*`，周期定时 `k_timer_start(timer,K_MSEC(10),K_MSEC(10))`。这些不是 FreeRTOS 同名API；`k_timer` expiry callback运行于系统时钟中断上下文。[kernel.h:515](/home/huiqin123/Git/zephyrproject/zephyr/include/zephyr/kernel.h:515)、同文件`:2013`、`:3816`、`:5488`、[timer文档:37](/home/huiqin123/Git/zephyrproject/zephyr/doc/kernel/services/timing/timers.rst:37)。

末表沿用 NuttX 报告的全部“功能”行。**现成**指实现或官方模块接入已存在；**需适配**包含目标引脚、数据格式、API或项目业务尚未对接；**缺失**限于本地主仓及manifest未检出，不代表第三方生态不存在。对比列依据既有 [NuttX报告末表:249](/home/huiqin123/esp/DesktopScreen/docs/survey-nuttx.md:249)，该报告中的部分应用层方案来自Vela apps。

| 功能 | Zephyr 现成方案（路径/compatible/Kconfig） | 状态（现成/需适配/缺失） | 对比 NuttX 的差异（一句话） |
|---|---|---|---|
| SSD1681 200×200、1bpp | `drivers/display/ssd16xx.c`；`solomon,ssd1681`；`CONFIG_SSD16XX`；有full/partial，缺项目计数策略，布局需对应 | 需适配 | 两者均有SSD1681；Zephyr这里已核实profile及局刷控制路径 |
| LCD／framebuffer 接口 | `include/zephyr/drivers/display.h`；`display_write()`；SSD16xx不提供直接framebuffer指针 | 现成 | 使用device/display API，接口形态不同于NuttX的`/dev/lcdN`、`/dev/fbN` |
| FT6336 | `drivers/input/input_ft5336.c`；`focaltech,ft5336`；`CONFIG_INPUT_FT5336`；有FT6336板级实例 | 需适配 | 比前报告多出明确FT6336实例，但中断方式和目标接线仍需对应 |
| SPI master | `drivers/spi/spi_esp32_spim.c`；`espressif,esp32-spi`；`CONFIG_ESP32_SPIM` | 现成 | 引脚与片选主要通过DTS/pinctrl描述 |
| I2C master | `drivers/i2c/i2c_esp32.c`；`espressif,esp32-i2c`；`CONFIG_I2C_ESP32` | 现成 | 同样有I2C0/1 master，接口为Zephyr I2C API |
| GPIO／GPIO 中断 | `drivers/gpio/gpio_esp32.c`；`CONFIG_GPIO_ESP32`；支持双沿和电平 | 现成 | 通过GPIO callback接入 |
| 项目 `/dev/gpioN` 引脚映射 | DTS GPIO spec/pinctrl；`include/zephyr/drivers/gpio.h` | 需适配 | Zephyr现成接入方式是设备API和DTS，不沿用该NuttX编号映射 |
| LEDC PWM | `drivers/pwm/pwm_led_esp32.c`；`espressif,esp32-ledc`；`CONFIG_PWM_LED_ESP32` | 现成 | 同有LEDC；Zephyr另有现成MCPWM驱动 |
| timer／one-shot／RTC | `drivers/counter/counter_esp32_{tmr,rtc}.c`；`CONFIG_COUNTER_TMR_ESP32/COUNTER_RTC_ESP32` | 现成 | 使用counter alarm/top API；RTC项为counter，不等同日历RTC |
| 原始 Flash 字库读取 | `subsys/storage/flash_map/`；`CONFIG_FLASH_MAP`；`PARTITION_ID`＋`flash_area_read` | 需适配 | 分区由DTS定义，读取为分区内偏移，默认表无1.48MB字库区 |
| SPIFFS | `subsys/fs/`、`west.yml`未检出SPIFFS实现或声明 | 缺失 | 前报告的NuttX树中有SPIFFS |
| LittleFS | `subsys/fs/littlefs_fs.c`；`CONFIG_FILE_SYSTEM_LITTLEFS`；外部模块未下载 | 现成 | 两边都有接入且依赖未展开 |
| FAT | `subsys/fs/fat_fs.c`；`CONFIG_FAT_FILESYSTEM_ELM`；外部FatFs模块未下载 | 现成 | Zephyr这里使用外部ELM FatFs，前报告NuttX是自身FAT实现 |
| NVS 类配置持久化 | `subsys/settings/`＋`subsys/kvss/{nvs,zms}/`；`CONFIG_SETTINGS_NVS/SETTINGS_ZMS` | 需适配 | 有字符串Settings层，API不同于IDF及NuttX configdata |
| ESP-IDF `nvs_*` 兼容层 | 主仓未检出；`hal_espressif`缺失，其内部不确定 | 缺失 | 两份调查均未确认可直接使用的IDF NVS兼容层 |
| Vela KVDB／UnQLite | 主仓、`west.yml`未检出；另有Settings/NVS/ZMS | 缺失 | 这是前报告Vela树已有、此Zephyr树未集成的组件 |
| WiFi STA／SoftAP／STA+AP | `drivers/wifi/esp32/`；`CONFIG_WIFI_ESP32/ESP32_WIFI_AP_STA_MODE`；`samples/net/wifi/apsta_mode/` | 现成 | 有双net_if和APSTA示例，且明确要求`!SMP` |
| WiFi 配置工具 | `subsys/net/l2/wifi/wifi_shell.c`；`CONFIG_NET_L2_WIFI_SHELL` | 现成 | 对应工具为Zephyr WiFi shell，而非WAPI |
| HTTP 客户端 | `subsys/net/lib/http/`；`CONFIG_HTTP_CLIENT`；`http_client_req()` | 现成 | 需先建立socket，再交给HTTP client |
| webclient HTTPS | 原生HTTP client＋TLS sockets；`samples/net/sockets/http_client/overlay-tls.conf` | 现成 | Zephyr sample已有TLS连接接法，前报告webclient缺已接好的TLS后端 |
| Vela libcurl HTTPS | 主仓、`west.yml`未检出libcurl；另有上述原生HTTPS路径 | 缺失 | 不提供前报告Vela的libcurl组件/API |
| cJSON | 主仓、manifest未集成；替代能力为`lib/utils/json.c`、`CONFIG_JSON_LIBRARY` | 缺失 | 前报告已有cJSON源码，Zephyr现成JSON接口是描述符式 |
| SNTP／NTP 校时功能 | `subsys/net/lib/sntp/`；`CONFIG_SNTP`；系统时钟设置另行调用 | 现成 | 查询与设时分开，CST-8不能对Minimal libc直接成立 |
| DHCP server | `subsys/net/lib/dhcpv4/dhcpv4_server.c`；`CONFIG_NET_DHCPV4_SERVER` | 现成 | 位于Zephyr主仓，APSTA示例已使用 |
| HTTP server | `subsys/net/lib/http/`；`CONFIG_HTTP_SERVER`；静态/动态资源sample | 现成 | 有嵌入网页和JSON POST示例，当前标为EXPERIMENTAL |
| 普通 captive DNS server | `subsys/net/lib/dns/`未检出UDP/53通配DNS服务器 | 缺失 | 两份调查均未找到该组件 |
| 完整 captive portal 配网业务 | APSTA＋DHCP＋HTTP组件已存在；项目四个POST、保存及页面业务需接入 | 需适配 | 两边均不是项目配网业务的直接成品；原项目实际未启用DNS劫持 |
| FreeRTOS 任务／队列／信号量调用 | `include/zephyr/kernel.h`；`k_thread/k_msgq/k_sem/k_timer` | 需适配 | Zephyr使用原生内核API，前报告主要对应task/pthread/mq/sem |
| LVGL | `modules/lvgl/`接入；`CONFIG_LVGL`；manifest固定提交，文档称9.6，模块未下载 | 现成 | 前报告Vela实际源码为9.1.0；此处还存在EPD分块flush与全刷的关联 |
| PC UI／触摸模拟 | `boards/native/native_sim/`；`CONFIG_SDL_DISPLAY/INPUT_SDL_TOUCH`；TAP或宿主sockets | 现成 | Zephyr使用SDL，前报告NuttX sim使用X11 |
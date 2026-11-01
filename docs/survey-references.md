> 开源参考项目调研，ZCode 于 2026-11-01 联网核实（GitHub API / 原始文件），本地部分已抽查。结论见 PLAN.md 第 6 节。

# 多系统移植参考调研报告

所有网络事实均于 **2026-11-01** 实际打开对应仓库/文档核实（证据链接随条目给出）；本地源码事实标注 `file:line`。凡未能核实的均明确标“未核实”。

## 1. 跨系统平台抽象层参考项目

### 1.1 WAMR（bytecodealliance/wasm-micro-runtime）——三系统全覆盖 ✅

- `core/shared/platform/` 下 **esp-idf、nuttx、zephyr 三个目录均存在**（我方独立抽查 + 子代理双重复核；同级还有 linux、freebsd、rt-thread、vxworks 等共 17 个平台目录）。证据：[platform 目录列表](https://api.github.com/repos/bytecodealliance/wasm-micro-runtime/contents/core/shared/platform)
- 平台层抽象内容（按头文件实际声明）分两层：
  - [platform_api_vmcore.h](https://raw.githubusercontent.com/bytecodealliance/wasm-micro-runtime/main/core/shared/platform/include/platform_api_vmcore.h)：`os_malloc/os_free`、`os_printf`、`os_mutex_*`、`os_time_get_boot_us`、`os_mmap/os_mprotect`（AOT/JIT）等；
  - [platform_api_extension.h](https://raw.githubusercontent.com/bytecodealliance/wasm-micro-runtime/main/core/shared/platform/include/platform_api_extension.h)：线程 `os_thread_create/join`、递归锁/读写锁/条件变量/命名信号量、原子宏、`os_usleep/os_clock_time_get`、**整套 socket（`os_socket_create/bind/accept/connect/recv/...`、`os_socket_addr_resolve`）**、**WASI 文件系统（`os_openat/os_preadv/os_fstat/...`）**。
- 平台目录规模差异本身有参考价值：[nuttx 平台](https://api.github.com/repos/bytecodealliance/wasm-micro-runtime/contents/core/shared/platform/nuttx)仅 3 个文件（大量复用 POSIX 实现）；[zephyr 平台](https://api.github.com/repos/bytecodealliance/wasm-micro-runtime/contents/core/shared/platform/zephyr) 8 个文件（zephyr_socket.c/thread.c/file.c/…）；[esp-idf 平台](https://api.github.com/repos/bytecodealliance/wasm-micro-runtime/contents/core/shared/platform/esp-idf) 9 个文件（espidf_socket.c/thread.c/…）。
- 许可证：Apache-2.0 + LLVM 例外条款（[LICENSE](https://raw.githubusercontent.com/bytecodealliance/wasm-micro-runtime/main/LICENSE)）。最近提交 2026-10-28（[commits](https://api.github.com/repos/bytecodealliance/wasm-micro-runtime/commits?per_page=1)）。

### 1.2 Matter（project-chip/connectedhomeip）——三系统全覆盖 ✅

- `src/platform/` 下 **ESP32、Zephyr、NuttX、Linux、FreeRTOS** 均存在（另有 android、nxp、silabs、telink、beken、ameba 等约 20 个平台）。证据：[src/platform 列表](https://api.github.com/repos/project-chip/connectedhomeip/contents/src/platform)
- 按平台实现的接口确认：`src/platform/ESP32/` 有 `KeyValueStoreManagerImpl.h(.cpp)` 和 `ConfigurationManagerImpl.cpp(.h)`（[ESP32 目录](https://api.github.com/repos/project-chip/connectedhomeip/contents/src/platform/ESP32)）；`src/platform/Zephyr/` 同样有这两个文件的 .h+.cpp（[Zephyr 目录](https://api.github.com/repos/project-chip/connectedhomeip/contents/src/platform/Zephyr)）。即“接口定义在 src/platform/include/（跨平台）+ 每平台一个目录实现同一组 Manager”的模式。
- 许可证：Apache-2.0（[LICENSE](https://raw.githubusercontent.com/project-chip/connectedhomeip/master/LICENSE)）。最近提交 2026-10-31。

### 1.3 Golioth Firmware SDK（golioth/golioth-firmware-sdk）——ESP-IDF + Zephyr，无 NuttX

- README 声称维护的平台移植：**ESP-IDF、Infineon ModusToolbox、Zephyr & NCS、Linux(实验)**；未提 NuttX（[README](https://raw.githubusercontent.com/golioth/golioth-firmware-sdk/main/README.md)）。`examples/` 实际为 `common/esp_idf/linux/modus_toolbox/zephyr` 五个目录。
- 抽象层目录：根目录 **`port/`（单数）** 下有 `esp_idf/freertos/linux/modus_toolbox/utils/zephyr` 六个子目录（[port/](https://api.github.com/repos/golioth/golioth-firmware-sdk/contents/port)）；另有顶层 `zephyr/` 作为 Zephyr module（含 module.yml）；`src/` 为平台无关核心（41 个文件），混入少量 `coap_client_zephyr.c`、`zephyr_coap_*.c` 平台特定文件。抽象对象：CoAP 客户端、系统（golioth_sys_*）、日志、OpenThread。
- 许可证：Apache-2.0。main 分支最近提交 2026-09-08（仓库整体 push 到 2026-10-28）。

### 1.4 Memfault Firmware SDK（memfault/memfault-firmware-sdk）——ESP-IDF + Zephyr，无 NuttX

- `ports/`（21 个目录）含 **esp_idf、freertos、zephyr**、mynewt、threadx、stm32cube 等，**无 nuttx**（[ports/](https://api.github.com/repos/memfault/memfault-firmware-sdk/contents/ports)）。
- 抽象内容：`components/` = core、http、metrics、panics、util 等；机制为“平台头文件定义必须实现的接口 + ports/ 提供移植模板”（[README](https://raw.githubusercontent.com/memfault/memfault-firmware-sdk/master/README.md)）。
- **许可证：自定义 "Memfault License"（BSD-2 结构 + 附加限制：仅可用于与 Memfault 服务集成、禁止逆向），非 OSI 开源**（[LICENSE](https://raw.githubusercontent.com/memfault/memfault-firmware-sdk/master/LICENSE)）。最近提交 2026-10-27（v1.46.0）。

### 1.5 MicroPython（micropython/micropython）——ESP-IDF + Zephyr，无 NuttX

- `ports/` 共 20 个目录，**esp32 ✓、zephyr ✓、无 nuttx**（[ports/](https://api.github.com/repos/micropython/micropython/contents/ports)）。
- NuttX 移植：**未找到可用的独立移植**。官方 issue [#13632 "Support for NuttX RTOS"](https://github.com/micropython/micropython/issues/13632)（2024-02 开，至今 Open）中维护者建议走 `ports/embed/`；NuttX 官方文档收的是 CPython 移植（[NuttX Python 文档](https://nuttx.apache.org/docs/latest/applications/interpreters/python/index.html)）。
- 许可证：MIT。最近提交 2026-10-30。

### 1.6 LVGL 在三个系统上的接入 + LVGL 9 单色支持 ✅（三系统全覆盖）

- **ESP-IDF**：`espressif/esp_lvgl_port` **独立仓库已 404**，组件（最新 2.9.0）并入 monorepo [espressif/esp-bsp](https://github.com/espressif/esp-bsp) 的 `components/esp_lvgl_port`，**Apache-2.0**（[license.txt](https://raw.githubusercontent.com/espressif/esp-bsp/master/components/esp_lvgl_port/license.txt)），该路径最近提交 2026-10-29。README 无 e-paper/eink 字样，`monochrome` 仅为配置字段名（[README](https://raw.githubusercontent.com/espressif/esp-bsp/master/components/esp_lvgl_port/README.md)）——面向 esp_lcd 面板，未声称支持墨水屏。
- **NuttX**：[apache/nuttx-apps/graphics/lvgl](https://api.github.com/repos/apache/nuttx-apps/contents/graphics/lvgl) 只是构建胶水（CMakeLists/Kconfig/Makefile 等 5 个文件，Apache-2.0）；真正的 NuttX 移植代码 `lv_nuttx_entry.c`、`lv_nuttx_fbdev.c`、`lv_nuttx_lcd.c`、`lv_nuttx_touchscreen.c` 等 15 个文件在 **LVGL 上游** [lvgl/lvgl/src/drivers/nuttx/](https://api.github.com/repos/lvgl/lvgl/contents/src/drivers/nuttx)。
- **Zephyr**：[zephyrproject-rtos/lvgl](https://github.com/zephyrproject-rtos/lvgl) 是 Zephyr 官方管理的 module（MIT，最新 v9.6.0，2026-09-16；[zephyr/module.yml](https://raw.githubusercontent.com/zephyrproject-rtos/lvgl/master/zephyr/module.yml)，登记于 zephyr [west.yml](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/west.yml)，path `modules/lib/gui/lvgl`）；官方 sample [samples/subsys/display/lvgl](https://api.github.com/repos/zephyrproject-rtos/zephyr/contents/samples/subsys/display/lvgl) 存在。
- **LVGL 9 的 1bpp/I1**：`LV_COLOR_FORMAT_I1 = 0x07` 存在（[v9.2.2 lv_color.h](https://raw.githubusercontent.com/lvgl/lvgl/v9.2.2/src/misc/lv_color.h)，注意 master 分支头文件已重构）；`lv_display_set_flush_cb` / `lv_display_set_buffers`（PARTIAL/DIRECT/FULL 渲染模式）存在（[v9.2.2 lv_display.h](https://raw.githubusercontent.com/lvgl/lvgl/v9.2.2/src/display/lv_display.h)）。官方文档 [Color Format 页](https://lvgl.io/docs/open/main-modules/display/color_format) 写明：I1 直接 1-bit 渲染、位序 MSB first、刷新区对齐到字节边界、I1 缓冲区多留 8 字节内嵌调色板（flush 里 `px_map += 8` 跳过）、提供 `lv_draw_i1_convert_to_vtiled` 做位图布局转换。**官方文档中未找到专门的 e-paper/局刷/关抗锯齿建议页**（“关抗锯齿 + DEPTH=1”的说法只出现在论坛/第三方教程，未核实为官方立场）。许可证 MIT（[LICENCE.txt](https://raw.githubusercontent.com/lvgl/lvgl/master/LICENCE.txt)），最近提交 2026-10-30。

### 1.7 u8g2（olikraus/u8g2）

- 许可证 BSD-2-Clause（[LICENSE](https://raw.githubusercontent.com/olikraus/u8g2/master/LICENSE)）。
- **不支持 SSD1681**：csrc/ 的 ssd* 系列为 ssd1305…ssd1363 + ssd1606_172x72 + ssd1607_200x200，电子纸控制器仅 **IL3820(296×128)、SSD1606(172×72)、SSD1607(200×200)** 三个（[csrc 列表](https://api.github.com/repos/olikraus/u8g2/contents/csrc)）。
- HAL 移植模式：u8x8 回调（`display_cb/cad_cb/byte_cb/gpio_and_delay_cb`，现成 `u8x8_byte_4wire_sw_spi` 等字节回调；[u8x8.h](https://raw.githubusercontent.com/olikraus/u8g2/master/csrc/u8x8.h)、官方移植指南 [Porting-to-new-MCU-platform](https://github.com/olikraus/u8g2/wiki/Porting-to-new-MCU-platform)）——纯回调、与 OS 无关。
- NuttX 主线（[nuttx-apps/graphics/](https://api.github.com/repos/apache/nuttx-apps/contents/graphics)）和 Zephyr west.yml 模块清单中**均无 u8g2**。最近提交时间：未核实。

### 1.8 其它（补充核实的两个）

- **wolfSSL**：仓库 license 字段 GPL-3.0，最近提交 2026-10-30。三系统痕迹齐备：根目录 `zephyr/`（module.yml/Kconfig/zephyr_init.c，[目录](https://api.github.com/repos/wolfSSL/wolfssl/contents/zephyr)）、`RTOS/nuttx/`（[目录](https://api.github.com/repos/wolfSSL/wolfssl/contents/RTOS/nuttx)）、`IDE/Espressif/`。
- **curl**：curl license（MIT 风格）；docs/INSTALL.md 的操作系统列表含 Zephyr（[INSTALL.md](https://raw.githubusercontent.com/curl/curl/master/docs/INSTALL.md)，CMake 层面支持未核实）；NuttX 侧 [netutils/](https://api.github.com/repos/apache/nuttx-apps/contents/netutils) 无 curl 目录，实际是 `libcurl4nx/`（curl4nx 项目）。

### 第 1 题汇总表

| 项目 | 链接 | ESP-IDF | NuttX | Zephyr | 抽象层目录 | 抽象了什么 | 许可证 | 最近提交 | 已打开核实 |
|---|---|:-:|:-:|:-:|---|---|---|---|:-:|
| WAMR | [repo](https://github.com/bytecodealliance/wasm-micro-runtime) | ✓ | ✓ | ✓ | `core/shared/platform/{esp-idf,nuttx,zephyr}` + `platform/include/platform_api_*.h` | malloc/日志/线程/互斥/读写锁/条件变量/信号量/原子/mmap/时间/socket/WASI 文件系统 | Apache-2.0(+LLVM例外) | 2026-10-28 | ✓ |
| Matter | [repo](https://github.com/project-chip/connectedhomeip) | ✓ | ✓ | ✓ | `src/platform/{ESP32,NuttX,Zephyr,...}`（接口在 src/platform/include） | KeyValueStoreManager/ConfigurationManager/NetworkCommissioning/ BLE 等按平台实现 | Apache-2.0 | 2026-10-31 | ✓ |
| Golioth SDK | [repo](https://github.com/golioth/golioth-firmware-sdk) | ✓ | ✗ | ✓ | `port/{esp_idf,zephyr,...}` + 顶层 `zephyr/` module | CoAP/系统(golioth_sys)/日志/时间 | Apache-2.0 | 2026-09-08(main) | ✓ |
| Memfault SDK | [repo](https://github.com/memfault/memfault-firmware-sdk) | ✓ | ✗ | ✓ | `ports/{esp_idf,zephyr,freertos,...}` + `components/*/include` 平台头 | metrics/core dump/日志/HTTP 上报 | 自定义 Memfault License（限制性，非 OSI） | 2026-10-27 | ✓ |
| MicroPython | [repo](https://github.com/micropython/micropython) | ✓ | ✗ | ✓ | `ports/{esp32,zephyr}`（整目录即移植层） | 完整解释器（非 OSAL 参考） | MIT | 2026-10-30 | ✓ |
| LVGL | [repo](https://github.com/lvgl/lvgl) | ✓ | ✓ | ✓ | ESP-IDF: esp-bsp `components/esp_lvgl_port`；NuttX: `lvgl/src/drivers/nuttx`；Zephyr: `zephyrproject-rtos/lvgl` module | 显示 flush/输入 indev 回调；v9 含 I1 1bpp | MIT（esp_lvgl_port 为 Apache-2.0） | 2026-10-30 | ✓ |
| u8g2 | [repo](https://github.com/olikraus/u8g2) | (无官方接入) | (无) | (无) | csrc/u8x8 回调（OS 无关） | 单色显示字节/GPIO 回调；不含 SSD1681 | BSD-2-Clause | 未核实 | ✓ |
| wolfSSL | [repo](https://github.com/wolfSSL/wolfssl) | ✓ | ✓ | ✓ | `zephyr/` module + `RTOS/nuttx` + `IDE/Espressif` | TLS/加密 + OS 抽象层 | GPL-3.0 | 2026-10-30 | ✓ |

## 2. 类似的成品项目（NuttX/Zephyr 墨水屏）

**NuttX 上：未找到任何成品墨水屏开源项目（0 个）**。NuttX 主线 e-paper 驱动仅 `drivers/lcd/ssd1680.c` 一个（[drivers/lcd](https://api.github.com/repos/apache/nuttx/contents/drivers/lcd)）。

**Zephyr 上核实通过的项目（4 个，按相关度）**：

1. [jcontrerasf/E-ink-display-Zephyr](https://github.com/jcontrerasf/E-ink-display-Zephyr) — Zephyr（PlatformIO 工程，nRF52840）；屏 **GDEH0154D67 1.54“ 200×200（控制器即 SSD1681，见第 3 题 GxEPD2 证据）**；代码含全刷 + 局刷（`EPD_Dis_Part`，时钟数字 32×32 局刷演示）+ 深睡眠；**无 LICENSE 文件**，仅 1 次提交（2022-07-17）。证据：[main.c](https://raw.githubusercontent.com/jcontrerasf/E-ink-display-Zephyr/master/src/main.c)
2. [xian-cong/nRF52840-dongle-e-paper-display](https://github.com/xian-cong/nRF52840-dongle-e-paper-display) — Zephyr（`west build -b nrf52840dongle_nrf52840`）；屏 Waveshare 1.54 V2（epd1in54_V2 驱动，200×200；V2=SSD1681 仅有间接证据，见第 3 题）；**无 LICENSE**，最近提交 2021-09-17。
3. [ikajdan/paperdeck](https://github.com/ikajdan/paperdeck) — Zephyr（XIAO ESP32-C3）的 Home Assistant/天气/日历墨水屏 dashboard；屏 Seeed 7.5“ 800×480 **UC8179**（非 SSD16xx）；MIT；最近提交 2026-07-19。
4. [Assar63/rv_display](https://github.com/Assar63/rv_display) — Zephyr（Nucleo-H753ZI，west 清单含 lvgl）；屏 Good Display 7.5” **UC8179**；无许可证信息；最近提交 2026-05-13。

**排除/负向结果**：未找到任何 “FT5x06/FT6x36 触摸 + 墨水屏” 的 NuttX/Zephyr 组合项目；[golioth/ostentus](https://github.com/golioth/ostentus) 虽有 `drivers/ssd1681`，但固件是 Pico 上的 MicroPython，非 Zephyr。

**顺带核实的主线驱动事实**（与本项目直接相关）：
- Zephyr `drivers/display/ssd16xx.c` 的 compatible 覆盖 ssd1608/1673/1675a/1680/**1681**/1683（各 [dts binding](https://github.com/zephyrproject-rtos/zephyr/tree/main/dts/bindings/display) 齐全）；主线还有 [reel_board](https://api.github.com/repos/zephyrproject-rtos/zephyr/contents/boards/phytec)（e-paper 参考板）和 [waveshare_epaper shield](https://api.github.com/repos/zephyrproject-rtos/zephyr/contents/boards/shields/waveshare_epaper)——其中 [gdeh0154a07.overlay](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/boards/shields/waveshare_epaper/waveshare_epaper_gdeh0154a07.overlay) 声明 `solomon,ssd1681`、200×200。
- 触摸：Zephyr 主线 `drivers/input/` 有 `input_ft5336.c` 和 `input_ft6146.c`，但 binding 只有 `focaltech,ft6146`（[focaltech,ft6146.yaml](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/dts/bindings/input/focaltech,ft6146.yaml)），**无 ft6336**；NuttX 主线 `drivers/input/` 有 `ft5x06.c`（[目录](https://api.github.com/repos/apache/nuttx/contents/drivers/input)）。

## 3. SSD1681 局部刷新的可复用实现

**本地两份驱动许可证（判断移植可行性的核心）：**
- Zephyr `drivers/display/ssd16xx.c`：**SPDX-License-Identifier: Apache-2.0**（`/home/huiqin123/Git/zephyrproject/zephyr/drivers/display/ssd16xx.c:6`）。且该驱动**已有成套局刷机制**：`SSD16XX_PROFILE_PARTIAL` profile（ssd16xx.c:42）、按 quirks 的 `ctrl2_partial` 更新控制字（ssd16xx.c:66）、`partial_keeps_ram` 等差异位（ssd16xx.c:72）、profile 结构含 LUT/GDV/SDV/vcom/dummy_line（ssd16xx.c:88-101）、由 devicetree 提供 full/partial 两套 profile（ssd16xx.c:112）。Apache-2.0 → Apache-2.0（NuttX）移植无许可证障碍。
- NuttX `drivers/lcd/ssd1680.c`：**SPDX-License-Identifier: Apache-2.0**（`/home/huiqin123/Git/nuttx/drivers/lcd/ssd1680.c:4`）。确认只有一套全刷 LUT（`ssd1680_lut`，ssd1680.c:293-307），无 partial window(0x90) 命令；更新路径为 `ssd1680_putrun` 写 RAM + `ssd1680_update_all_and_redraw`（ssd1680.c:221）整体刷新——与“只支持全刷”的判断一致。

**其它 SSD1681 局刷实现及许可证（兼容性结论按“能否把代码复制进 Apache-2.0 项目而不改变其许可证”判定）：**

| 实现 | 许可证（证据） | 与 Apache-2.0 的兼容性 | SSD1681 相关 |
|---|---|---|---|
| [Zephyr ssd16xx.c](https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/display/ssd16xx.c) | Apache-2.0 | 兼容 | ssd1681 compatible + profile 局刷 |
| [libdriver/ssd1681](https://api.github.com/repos/libdriver/ssd1681) | MIT | 兼容 | 独立完整驱动（MCU/Linux），2026-10-26 仍活跃 |
| [adafruit/Adafruit_EPD](https://api.github.com/repos/adafruit/Adafruit_EPD) | MIT | 兼容 | 支持 SSD1680/SSD1681 系，2026-10-23 仍活跃 |
| [GxEPD2](https://raw.githubusercontent.com/ZinggJM/GxEPD2/master/LICENSE) | **GPL-3.0** | **不兼容**（GPL 代码并入 Apache-2.0 项目会迫使衍生部分以 GPL 发布；反向才成立） | `GxEPD2_154_D67` 类 200×200，头文件注明 Panel GDEH0154D67 / Controller SSD1681（[GxEPD2_154_D67.h](https://raw.githubusercontent.com/ZinggJM/GxEPD2/master/src/epd/GxEPD2_154_D67.h)） |
| [waveshareteam/e-Paper](https://api.github.com/repos/waveshare/e-Paper)（waveshare/e-Paper 已更名） | **仓库无 LICENSE 文件**（默认保留所有权利；部分 Python 源文件头有 Waveshare MIT 声明，[如 epd1in54_V2.py](https://raw.githubusercontent.com/waveshareteam/e-Paper/master/RaspberryPi_JetsonNano/python/lib/waveshare_epd/epd1in54_V2.py)） | 仓库层面无法判定（无许可证 = 不可复制；文件级 MIT 的仅个别文件） | `Arduino/epd1in54_V2`（[epd1in54_V2.cpp](https://raw.githubusercontent.com/waveshareteam/e-Paper/master/Arduino/epd1in54_V2/epd1in54_V2.cpp)）含 `WF_PARTIAL_1IN54_0` 局刷 LUT + 0x24/0x26 写 RAM；"V2=SSD1681"为间接证据（GxEPD2 映射 `Waveshare_1_54_bw_D67 = GDEH0154D67`；数据手册引述未直接打开） |
| [ESPHome](https://raw.githubusercontent.com/esphome/esphome/dev/LICENSE) | **双许可 MIT + GPLv3**（C++ 运行时为 GPLv3） | C++ 侧**不兼容** | **无 ssd1681/ssd1680/ssd1675 组件**；e-paper 平台只有 waveshare_epaper/epaper_spi/it8951/inkplate（[display 组件列表](https://esphome.io/components/display/)） |
| [EPDiy](https://raw.githubusercontent.com/vroland/epdiy/master/LICENSE) | LGPL-3.0 | 复制源码进 Apache-2.0 项目不兼容（LGPL 义务仍触发） | 面向大尺寸**并口**屏（4.7–13.3“），README 未提及 SSD1681，不覆盖 SPI 小屏 |
| [gooddisplay/EPD-ESP32-Arduino-IDE](https://github.com/gooddisplay) | Apache-2.0（该仓库） | 兼容 | 但 GoodDisplay 组织内未找到 1.54”/SSD1681 专门 demo（未核实到） |

## 4. POSIX 覆盖面（网络 + 线程/时间放 core 的可行性事实）

### 4.1 ESP-IDF v5.4（本地源码为 v5.4.1，`components/esp_common/include/esp_idf_version.h:14-18`）

| POSIX 能力 | 支持情况 | 证据 |
|---|---|---|
| pthread | 原生实现：`pthread_create` 等在 components/pthread/pthread.c；含 cond/rwlock/semaphore/TLS | `components/pthread/`（pthread.c、pthread_cond_var.c、pthread_rwlock.c 等） |
| BSD socket | 支持（lwIP 提供标准名） | `components/lwip/port/esp32xx/include/sys/socket.h` |
| select | 支持，且与 VFS 打通（可同时等 socket + VFS fd） | `components/vfs/vfs.c:1819`（`select()` 实现）；Kconfig `VFS_SUPPORT_SELECT`（components/vfs/Kconfig:41） |
| poll | 支持（基于 select 实现） | `components/newlib/poll.c:14`；头 `components/newlib/platform_include/sys/poll.h` |
| VFS | 原生（esp_vfs_register 等） | `components/vfs/`（esp_vfs.h） |
| mqueue | 支持（FreeRTOS-Plus-POSIX 移植，`rt` 组件） | `components/rt/include/mqueue.h:88`（mq_open）；`components/rt/FreeRTOS_POSIX_mqueue.c` |
| clock_gettime / settimeofday | 均实现 | `components/newlib/time.c:249` / `time.c:184` |
| SNTP | lwIP apps 自带 | `components/lwip/apps/sntp/` |

### 4.2 Zephyr v4.5.0（本地 `VERSION` 确认 4.5.0）

- POSIX 子系统已移到 `subsys/portability/posix/`（4.x 起，非旧的 subsys/posix）。
- **`CONFIG_POSIX_API` 仍存在**（`subsys/portability/posix/Kconfig.profile:5`），= 一键选 POSIX_SYSTEM_INTERFACES + PSE51（线程/时钟/信号量等）+ NETWORKING；帮助文本注明未来可能被细分选项取代。也可按 POSIX 选项组精选：`POSIX_THREADS`（options/Kconfig.pthread:7）、`POSIX_MESSAGE_PASSING`（mqueue，Kconfig.mqueue:5，实现在 options/mqueue.c）、`POSIX_FD_MGMT`（open/read/write）、`POSIX_FILE_SYSTEM` 等；另有 PSE51/PSE52/PSE53/NETAPP 子档位 choice。
- **socket 标准 API 名**：`CONFIG_NET_SOCKETS_POSIX_NAMES` 已废弃（migration-guide-3.7.rst:837：要么开 POSIX_API 用标准名，要么用 `zsock_` 前缀）；4.5 里对应的是 **`CONFIG_POSIX_NETWORKING`**（options/Kconfig.net:5，select NET_SOCKETS，注释明确"support for BSD Sockets"）。
- 时间：`clock_gettime/clock_settime/nanosleep/usleep` 已实现（options/clock.c:16/59/81/101）；`gettimeofday` 已实现（options/xsi_single_process.c:39）；**`settimeofday` 在 4.5 树中未找到实现（grep portability/lib/include 无结果）**——即 core 若用 settimeofday 对接 SNTP，在 Zephyr 上无此 POSIX 符号；Zephyr 自带 `subsys/net/lib/sntp/`（sntp.c/sntp_simple.c，非 POSIX 接口）。
- select：`select()/pselect()` 已实现（options/device_io.c:151/125，包装 zvfs_select）；`include/zephyr/posix/` 下有 poll.h、mqueue.h、netdb.h、pthread.h、unistd.h 等标准路径头。

### 4.3 NuttX（本地 `/home/huiqin123/Git/nuttx`）

原生 POSIX，无需开关：`include/pthread.h` + `sched/pthread/`（pthread）、`include/mqueue.h`（mqueue）、`include/sys/socket.h`、`include/netdb.h`、`include/netinet/in.h`（socket/DNS）、`sched/clock/clock_gettime.c`（clock_gettime）、`libs/libc/time/lib_settimeofday.c`（settimeofday，已实现——与 Zephyr 形成差异点）。HTTP/SNTP 等工具在 nuttx-apps 的 netutils（本地未检出到 apps 仓库；线上核实 netutils 存在 http/ 等，curl 相关为 curl4nx）。

### 4.4 三系统能力对照（事实小结）

| 能力 | ESP-IDF 5.4.1 | Zephyr 4.5.0 | NuttX |
|---|:-:|:-:|:-:|
| pthread | ✓ | ✓（CONFIG_POSIX_THREADS） | ✓ |
| BSD socket 标准名 | ✓ | ✓（CONFIG_POSIX_NETWORKING，POSIX_NAMES 已废弃） | ✓ |
| select / poll | ✓ | ✓ / ✓（poll.h 存在） | ✓ |
| mqueue | ✓ | ✓（POSIX_MESSAGE_PASSING） | ✓ |
| clock_gettime | ✓ | ✓ | ✓ |
| **settimeofday** | ✓ | **未实现** | ✓ |
| SNTP | lwip/apps/sntp | subsys/net/lib/sntp（自有 API） | nuttx-apps netutils |

---

**主要未核实项汇总**：u8g2 最近提交时间；curl 的 CMake 级 Zephyr 支持；Waveshare 1.54 V2 控制器型号（仅间接证据）；LVGL 官方文档中无 e-paper 专项建议页（关抗锯齿等说法非官方）；GoodDisplay 专门的 SSD1681 demo 仓库。

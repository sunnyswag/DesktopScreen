# DesktopScreen

基于 ESP32 的桌面墨水屏：1.54 寸 SSD1681 墨水屏（200×200）+ FT6336 触摸，功能包括时钟、天气、番茄钟、单词卡，以及通过 AP 网页配网。

> 原始代码来自**小智学长**的公开教程。本仓库在它的基础上做学习记录，后续会迁移到 NuttX，见 [desktopscreen-nuttx](https://github.com/sunnyswag/desktopscreen-nuttx)。

## 分支

| 分支 / 标签 | 说明 |
|---|---|
| `dev0` … `dev_17_all_d67`、`v1.0` … `v2.6` | 教程原始的逐步开发分支和标签（基于 ESP-IDF 4.4） |
| `main` | 在 `dev_17_all_d67` 基础上清理了死代码，并适配 ESP-IDF 5.4.1 |

## 编译与烧录（main 分支，ESP-IDF v5.4.1）

```bash
. $HOME/esp/esp-idf/export.sh         # 配置环境
idf.py build                          # 编译，成功会提示 Project build complete.
idf.py -p /dev/ttyUSB0 flash monitor  # 烧录并打开串口监视器（Ctrl+] 退出）
```

其它常用命令：

```bash
idf.py menuconfig                     # 配置
idf.py fullclean                      # 清除编译产物
idf.py -p /dev/ttyUSB0 erase-flash    # 擦除整片 Flash
```

## 字库烧录

中文字库 `myFont1.bin` 单独烧到 Flash 的 `storage1` 分区（偏移 0x15D000）。只需要烧一次，`erase-flash` 之后要重新烧：

```bash
python esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 write_flash -z 0x15D000 myFont1.bin
```

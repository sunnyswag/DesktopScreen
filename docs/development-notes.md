# 桌面小屏幕项目开发记录

本文由个人笔记《桌面小屏幕项目开发》整理而来，汇总项目资料、学习过程与后续计划。当前 `main` 分支的编译和烧录步骤见 [README](../README.md#编译与烧录main-分支esp-idf-v541)。

## 项目资料

- [项目参考视频](https://www.bilibili.com/video/BV1wV4y1G7Vk?t=4.4)
- [桌面小屏幕实战课程资料文档](https://x509p6c8to.feishu.cn/docx/NQCTdjUFJoYoZ1xYHS9cIlbwnxh)
- [桌面小屏幕：快速上手·硬件章](https://x509p6c8to.feishu.cn/docx/ZKUDdD9VhoNroXxdbP6c8GyFn3f)
- [烧录固件说明](https://x509p6c8to.feishu.cn/docx/doxcnog27XGKtClItsVBZK4f8ML)
- [ESP-IDF 开发环境配置文档](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/esp32/get-started/linux-macos-setup.html#get-started-linux-macos-first-steps)

ESP-IDF 链接指向官方 `stable` 文档；本仓库 `main` 分支使用 ESP-IDF **v5.4.1**，教程原始分支基于 **4.4**，以 [README 的分支说明](../README.md#分支) 为准。

## 学习与采购记录

原笔记记录的时间为 **2025-03-05 至 2025-03-09**，投入 **6 小时**，`process` 字段为 `8`，状态为 `Done`。以下清单保留当时的勾选状态，作为历史记录。

- [x] 查看前几期视频，了解相关基础
- [ ] 购买材料
  - [x] 嘉立创下单板子
  - [x] 电池
  - [x] 屏幕
  - [ ] 外壳
  - [x] 其他器件
- [ ] 学习并开发
- [ ] 使用 NuttX 重新开发同样的功能
- [ ] 学习 PCB Layout

后来购买了一块已经制作好的板子，花费 **80 元**，先开始软件部分的学习，硬件焊接留待以后有时间再做。

### 嘉立创打样资料

当时记录的打样流程如下，优惠活动与领取条件以平台实际页面为准：

1. 在 [嘉立创优惠券页面](https://www.jlc.com/newOrder/#/collectCoupons?spm=JLC.MEMBER) 领取优惠券；原笔记记录为答题及格后可领取。
2. 下载嘉立创下单助手。
3. 参考 [嘉立创 EDA 免费打板教程](https://www.bilibili.com/video/BV14GUrYdESX?t=223.9) 下单。

## WSL2 串口配置

学习时使用 [usbipd-win](https://github.com/dorssel/usbipd-win) 将 Windows 下的设备连接到 WSL2。以下命令在 Windows PowerShell 中执行，将 `<BUSID>` 替换为目标设备的总线 ID：

```powershell
usbipd bind --busid=<BUSID>
usbipd attach --wsl --busid=<BUSID>
```

设备连接后，在 WSL2 中按实际串口路径配置读写权限。原笔记使用的串口是 `/dev/ttyUSB0`，临时授权命令为：

```bash
sudo chmod 666 /dev/ttyUSB0
```

### 常用 ESP-IDF 命令

在仓库根目录执行。当前 `main` 分支的环境初始化方式与 [README](../README.md#编译与烧录main-分支esp-idf-v541) 一致：

```bash
. "$HOME/esp/esp-idf/export.sh"         # 加载 ESP-IDF 环境
idf.py build                          # 编译
idf.py -p /dev/ttyUSB0 flash monitor   # 烧录并打开串口监视器
idf.py menuconfig                     # 配置
idf.py fullclean                      # 清除编译产物
```

原笔记使用 `get_idf` 初始化环境；它是本地配置的快捷命令，使用前需先配置对应别名或函数。字库需要单独烧录，步骤见 [README 的字库烧录说明](../README.md#字库烧录)。

## 首次使用：网页配网与连接排查

如果设备连不上 Wi-Fi，串口反复打印 `retry to connect to the AP`，且尚未通过网页配网，很可能还在尝试连接教程作者小智学长的默认 Wi-Fi。当前 `sdkconfig` 中的默认配置为：

```text
CONFIG_ESP_WIFI_SSID="leo"
CONFIG_ESP_WIFI_PASSWORD="123456789"
```

通过设备提供的网页设置自己的 Wi-Fi：

1. 手机连接设备热点 **桌面屏幕**。当前代码将其设置为开放热点，无需密码。
2. 在手机浏览器打开 **http://192.168.4.1**。
3. 填入自己的 Wi-Fi 名称和密码，提交设置。
4. 设备将配置保存到 NVS，并更新 Wi-Fi 连接配置，随后尝试连接新的网络。重启后也会优先读取 NVS 中保存的配置。

连接成功时，串口会出现 `connect success ! got ip:`，后面跟随设备获得的 IP 地址；由默认账号不匹配引起的重试日志应停止。网页提示“WiFi设置成功！”表示配置已提交，是否实际连上路由器以串口连接成功日志为准。

如果提交后仍未连接，可重启设备，让它重新加载 NVS 中的配置；若仍反复重试，再检查填写的 Wi-Fi 名称、密码和路由器是否可用。执行 `erase-flash` 会清除 NVS，之后需要重新配网。

## 下一步：用 NuttX 重写

这一方向已纳入个人笔记《视频项目候选清单》的项目 1，计划完成后制作一期视频：

- 将 SPI 屏幕和按键驱动从 ESP-IDF 迁移到 NuttX。
- 学习 NuttX 上半部分（对接 VFS）和下半部分（操作硬件）的驱动模型。
- 使用 NuttX 重新实现同样的功能，并记录迁移过程。

迁移仓库：[desktopscreen-nuttx](https://github.com/sunnyswag/desktopscreen-nuttx)。

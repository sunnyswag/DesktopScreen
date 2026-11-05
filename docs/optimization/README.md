# 代码优化记录

记录优化前后的资源、性能和代码状态，方便对比每轮优化的效果。

| 文档 | 内容 |
|---|---|
| [methodology.md](methodology.md) | 评估方法：环境、指标定义、测试剧本、脚本用法、探针原理、注意事项 |
| [baseline-2026-11-04.md](baseline-2026-11-04.md) | 优化前基线（提交 `6f09703`）：Flash、RAM、CPU、功耗、启动、代码量、架构、健壮性、安全 |
| [data/](data/) | 每次评估的 summary.md 和 metrics.json，按 `<日期>-<提交>` 分目录 |

脚本在 [tools/perf/](../../tools/perf/)；发现的问题记在 GitHub issue [#1](https://github.com/sunnyswag/DesktopScreen/issues/1)–[#25](https://github.com/sunnyswag/DesktopScreen/issues/25)，标签 `optimization`。

## 优化方向

| 方向 | 相关 issue |
|---|---|
| Flash 占用 | [#1](https://github.com/sunnyswag/DesktopScreen/issues/1) 配置裁剪、[#2](https://github.com/sunnyswag/DesktopScreen/issues/2) SPIFFS 和分区、[#3](https://github.com/sunnyswag/DesktopScreen/issues/3) 字库、[#4](https://github.com/sunnyswag/DesktopScreen/issues/4) 图片数组 |
| RAM 占用 | [#4](https://github.com/sunnyswag/DesktopScreen/issues/4) 静态缓冲、[#25](https://github.com/sunnyswag/DesktopScreen/issues/25) 常驻堆和栈余量、[#5](https://github.com/sunnyswag/DesktopScreen/issues/5) 任务栈 |
| CPU | [#14](https://github.com/sunnyswag/DesktopScreen/issues/14) 10 ms 定时器、[#15](https://github.com/sunnyswag/DesktopScreen/issues/15) SPI 传输、[#16](https://github.com/sunnyswag/DesktopScreen/issues/16) 无效工作 |
| 功耗 | [#17](https://github.com/sunnyswag/DesktopScreen/issues/17) 休眠和电源管理、[#18](https://github.com/sunnyswag/DesktopScreen/issues/18) WiFi 模式、[#12](https://github.com/sunnyswag/DesktopScreen/issues/12) 唤醒处理 |
| 代码架构 | [#21](https://github.com/sunnyswag/DesktopScreen/issues/21) 分层和全局状态、[#22](https://github.com/sunnyswag/DesktopScreen/issues/22) 弃用驱动、[#23](https://github.com/sunnyswag/DesktopScreen/issues/23) 清理 |
| 健壮性（新增） | [#5](https://github.com/sunnyswag/DesktopScreen/issues/5)–[#13](https://github.com/sunnyswag/DesktopScreen/issues/13) |
| 安全（新增） | [#19](https://github.com/sunnyswag/DesktopScreen/issues/19)、[#20](https://github.com/sunnyswag/DesktopScreen/issues/20) |
| 启动与响应（新增） | [#24](https://github.com/sunnyswag/DesktopScreen/issues/24) |

## 关键指标

每轮优化后补一列。完整指标见各次的 `data/*/metrics.json`，用 `python3 tools/perf/compare.py` 生成全量对比。

| 指标 | 基线 2026-11-04（`6f09703`） | 优化后 |
|---|---|---|
| app.bin / app 分区余量 | 987,440 B / 61,136 B（6%） | |
| 静态 DRAM / IRAM | 55,360 B（30.6%）/ 103,658 B（79.1%） | |
| 运行时堆最低剩余（测量固件，自动剧本） | 127,032 B（扣除探针开销约 136 KB） | |
| 最紧的业务任务栈 | gpio_task_example 剩 172 B（模拟触摸） | |
| 业务 CPU 负载（两核合计）：空闲 / 翻页 | 0.97% / 1.96% | |
| 定时唤醒频率 | 10 ms 中断 + 2 个任务各 100 次/秒 | |
| 进入 light sleep 的条件 | APSTA 关闭 + 在主页 + 无操作 600 s（触摸后 60 s） | |
| STA 拿到 IP / 首次刷屏完成 | 3.86 s / 2.95 s | |
| 整屏全刷耗时（主页 / 时间页） | 2.48 s / 2.57 s | |
| NLOC / 函数 / 平均 CCN / 最大 CCN | 3,966 / 180 / 2.95 / 29 | |
| CCN > 15 的函数 / 长度 > 60 NLOC 的函数 | 5 / 7 | |
| 编译告警（项目代码 / 合计） | 3 / 5 | |
| 未关闭的 optimization issue | 25 | |

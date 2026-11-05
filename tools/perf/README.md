# tools/perf — 资源与性能评估脚本

优化前后用同一套脚本评估，结果用 `compare.py` 对比。测试方法和指标定义见 [docs/optimization/methodology.md](../../docs/optimization/methodology.md)，基线结论见 [docs/optimization/](../../docs/optimization/)。

| 脚本 | 测什么 | 要不要设备 | 耗时 |
|---|---|---|---|
| `size_report.sh` | Flash/RAM 各段、分区余量、组件排行、图片数组、编译告警 | 否 | 3~5 分钟 |
| `size_variants.sh` | sdkconfig 对照实验（各组定义在 `variants.txt`） | 否 | 10~15 分钟 |
| `code_metrics.sh` | NLOC、圈复杂度、长函数、重复代码、日志调用 | 否 | 几秒 |
| `device_probe.sh` | 运行时堆、任务栈、CPU 占用、启动时间线、刷屏/HTTP/睡眠事件 | 是 | 约 8 分钟，剧本自动执行（`--manual` 改为人工操作） |
| `compare.py` | 两次结果的对比表 | 否 | 几秒 |

```bash
# 评估某个提交（默认 HEAD），结果写到 perf-out/<日期>-<短哈希>/
bash tools/perf/size_report.sh --ref HEAD
bash tools/perf/code_metrics.sh --ref HEAD
bash tools/perf/device_probe.sh --ref HEAD --port /dev/ttyUSB0
# 评估当前工作区（含未提交改动）
bash tools/perf/size_report.sh --worktree
# 对比
python3 tools/perf/compare.py perf-out/<基线目录> perf-out/<新目录> > perf-out/compare.md
```

- 被测代码总是导出到结果目录下单独编译，不会动仓库里的 `build/` 和 `sdkconfig`。
- `perf-out/` 已在 `.gitignore` 里忽略。要入库的结论整理到 `docs/optimization/`；`device/probe.log` 可能含 WiFi 密码，不要提交。
- `device_probe.sh` 只写设备的 app 分区，采集完写回原固件并核对版本；运行前关掉 `idf.py monitor`。
- 剧本自动执行：探针模拟触摸芯片完成翻页；配网页默认由电脑通过局域网请求（`--http host`，要求电脑和设备同网段），也可以 `--http loopback` 由设备自己请求。偏差说明见 methodology.md 第 3 节。
- 探针组件 `probe/perf_probe/` 通过链接器 `--wrap` 注入，业务代码不需要改。被测代码里改名或删掉的函数只是不再出现对应事件，不影响编译；新增要统计的函数时，在 `probe/perf_probe/CMakeLists.txt` 和 `perf_wrap.c` 里加。

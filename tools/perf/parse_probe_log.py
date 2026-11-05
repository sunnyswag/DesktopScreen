"""device_probe.sh 的解析脚本：parse_probe_log.py <结果目录> <被测代码描述>

读取 <结果目录>/probe.log 里 perf_probe 输出的 "PERF " 行（格式见 probe/perf_probe/perf_probe.c），
按 scenario.txt 的阶段统计 CPU，写出 summary.md 和 metrics.json。输出里不包含原始日志内容。
"""
import json
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path

PERF_DIR = Path(__file__).resolve().parent
WRAP = 1 << 32
STACK_RISK_BYTES = 512
ANSI = re.compile(r"\x1b\[[0-9;]*m")
RE_KV = re.compile(r"(\w+)=(-?\d+)")
RE_TASK = re.compile(r"PERF task t_ms=(\d+) rt=(\d+) hwm=(\d+) stack=(\d+) prio=(\d+) core=(-?\d+) name=(.{1,15}?)\s*$")
RE_SPAN = re.compile(r"PERF span t_ms=(\d+) dur_us=(\d+) arg=(-?\d+) name=(\w+)(?: chk=(\d+))?\s*$")
HEAP_KEYS = ("free8", "min8", "largest8", "int_free", "int_min", "total8")


def kv(line):
    return {k: int(v) for k, v in RE_KV.findall(line)}


def load_scenario():
    phases = []
    for line in open(PERF_DIR / "scenario.txt", encoding="utf-8"):
        line = line.strip()
        if line and not line.startswith("#"):
            at, text = line.split(maxsplit=1)
            phases.append((float(at) * 1000, text.split()[0] if text.startswith("阶段") else text))
    return phases


def sdk_int(sdkconfig, key, default=0):
    m = re.search(rf"^{key}=(\d+)", sdkconfig, re.M)
    return int(m.group(1)) if m else default


def ms(x):
    return "-" if x is None else f"{x / 1000:.2f} s"


def main():
    out, source_desc = Path(sys.argv[1]), sys.argv[2]
    text = ANSI.sub("", (out / "probe.log").read_bytes().decode("utf-8", errors="replace"))
    lines = text.splitlines()
    sdkconfig = (out / "src" / "sdkconfig").read_text()
    builtin_stacks = {
        "IDLE0": sdk_int(sdkconfig, "CONFIG_FREERTOS_IDLE_TASK_STACKSIZE"),
        "IDLE1": sdk_int(sdkconfig, "CONFIG_FREERTOS_IDLE_TASK_STACKSIZE"),
        "Tmr Svc": sdk_int(sdkconfig, "CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH"),
    }

    # 串口在主机侧会丢字节：每组采样要条数齐全、校验和一致才采用（第 1 版探针没有校验和，只核对条数）
    hello, heaps, spans = {}, [], []
    samples = []  # [(t_ms, rt_total, {name: (rt, hwm, stack, prio, core)})]
    pending = None  # [t_ms, rt_total, count, tasks]
    dropped = {"heap": 0, "tasks": 0, "span": 0}

    def finish(end=None):
        nonlocal pending
        if pending is None:
            return
        t, rt_total, count, tasks = pending
        ok = len(tasks) == count
        if end is not None:
            ok = ok and end.get("t_ms") == t and sum(v[0] for v in tasks.values()) % WRAP == end.get("rt_sum") \
                and sum(v[1] for v in tasks.values()) % WRAP == end.get("hwm_sum")
        if ok:
            samples.append((t, rt_total, tasks))
        else:
            dropped["tasks"] += 1
        pending = None

    for line in lines:
        if "PERF hello" in line:
            hello = kv(line)
        elif "PERF heap" in line:
            finish() if hello.get("version", 1) < 2 else None
            d = kv(line)
            if all(k in d for k in HEAP_KEYS) and ("chk" not in d or sum(d[k] for k in HEAP_KEYS) % WRAP == d["chk"]):
                heaps.append(d)
            else:
                dropped["heap"] += 1
        elif "PERF tasks_end" in line:
            finish(kv(line))
        elif "PERF tasks" in line:
            finish()
            d = kv(line)
            pending = [d.get("t_ms"), d.get("rt_total"), d.get("count"), {}] if {"t_ms", "rt_total", "count"} <= d.keys() else None
        elif "PERF task " in line:
            m = RE_TASK.search(line)
            if pending and m and int(m.group(1)) == pending[0]:
                t, rt, hwm, stack, prio, core, name = m.groups()
                pending[3][name] = (int(rt), int(hwm), int(stack) or builtin_stacks.get(name, 0), int(prio), int(core))
        elif "PERF span" in line:
            m = RE_SPAN.search(line)
            if m and (m.group(5) is None or (int(m.group(1)) + int(m.group(2))) % WRAP == int(m.group(5))):
                spans.append({"t": int(m.group(1)), "dur_us": int(m.group(2)), "arg": int(m.group(3)), "name": m.group(4)})
            else:
                dropped["span"] += 1
    finish()
    if not hello or len(samples) < 2:
        sys.exit("probe.log 里没有足够的 PERF 输出，检查测量固件是否正常启动")

    fw = re.search(r"App version:\s*(\S+)", text)
    spiffs = re.search(r"Partition size: total: (\d+), used: (\d+)", text)
    crashes = len(re.findall(r"Guru Meditation|abort\(\) was called|Stack canary watchpoint|stack overflow", text))
    reboots = max(0, len(re.findall(r"rst:0x[0-9a-f]+ \(", text)) - 1)

    first = lambda name: next((s for s in spans if s["name"] == name), None)
    metrics = {}

    # 启动时间线（毫秒，以 esp_timer 启动为起点，约比芯片复位晚 0.5 秒）
    scr = next((s for s in spans if s["name"].startswith("screen_")), None)
    http = first("http_perform")
    boot = {
        "boot_app_main_ms": hello.get("app_main_ms"),
        "boot_got_ip_ms": first("got_ip")["t"] if first("got_ip") else None,
        "boot_time_valid_ms": first("time_valid")["t"] if first("time_valid") else None,
        "boot_first_screen_done_ms": scr["t"] + scr["dur_us"] // 1000 if scr else None,
        "boot_first_http_done_ms": http["t"] + http["dur_us"] // 1000 if http else None,
    }
    metrics.update(boot)

    # 堆
    probe_overhead = hello.get("static_bytes", 0) + hello.get("dynamic_bytes", 0)
    phases = load_scenario()
    idle_start = next((t for t, name in phases if name.startswith("阶段4")), phases[-2][0])
    idle_free = [h["free8"] for h in heaps if h["t_ms"] >= idle_start] or [heaps[-1]["free8"]]
    metrics.update(
        heap_total8=heaps[0]["total8"], heap_free_boot=heaps[0]["free8"],
        heap_free_idle_median=int(statistics.median(idle_free)), heap_min8=heaps[-1]["min8"],
        heap_int_min=heaps[-1]["int_min"], heap_largest_min=min(h["largest8"] for h in heaps),
        probe_overhead_bytes=probe_overhead, heap_min8_without_probe=heaps[-1]["min8"] + probe_overhead,
    )

    # 任务栈：每个任务取采样期间的最小水位
    stacks = {}
    for _, _, tasks in samples:
        for name, (rt, hwm, stack, prio, core) in tasks.items():
            old = stacks.get(name)
            stacks[name] = (min(hwm, old[0]) if old else hwm, stack or (old[1] if old else 0), prio, core)
    biz = {n: v for n, v in stacks.items() if n != "perf_probe"}
    worst = min(biz.items(), key=lambda kv_: kv_[1][0])
    metrics.update(
        tasks_count=len(biz), stack_min_margin_bytes=worst[1][0], stack_min_margin_task=worst[0],
        stack_total_alloc_bytes=sum(v[1] for v in biz.values()),
        stack_total_unused_bytes=sum(v[0] for v in biz.values() if v[1]),
        stack_risk_tasks=sum(1 for v in biz.values() if v[0] < STACK_RISK_BYTES),
    )

    # CPU：相邻两次采样之间，各任务运行时间增量 / (窗口时长 × 2 核)
    windows = []  # (t_end_ms, d_total, {name: d})
    for (t0, tot0, a), (t1, tot1, b) in zip(samples, samples[1:]):
        d_total = (tot1 - tot0) % WRAP
        if d_total == 0:
            continue
        windows.append((t1, d_total, {n: (b[n][0] - a[n][0]) % WRAP for n in b if n in a}))

    # 探针固定在某个核上时（第 2 版起固定在 core1），从该核负载里扣掉探针时间；业务负载 = 非 IDLE、非探针任务时间 / 两核总时间
    probe_core = next((t[4] for _, _, ts in samples for n, t in ts.items() if n == "perf_probe"), -1)

    def load(win):
        total = sum(w[1] for w in win)
        if not total:
            return None
        busy = {}
        probe = sum(w[2].get("perf_probe", 0) for w in win)
        for core, idle in ((0, "IDLE0"), (1, "IDLE1")):
            idle_t = sum(w[2].get(idle, 0) for w in win) + (probe if core == probe_core else 0)
            busy[f"core{core}"] = 100 * (1 - idle_t / total)
        biz = sum(d for w in win for n, d in w[2].items() if n not in ("IDLE0", "IDLE1", "perf_probe"))
        busy["business"] = 100 * biz / (2 * total)
        busy["probe"] = 100 * probe / (2 * total)
        busy["unaccounted"] = 100 * (1 - sum(sum(w[2].values()) for w in win) / (2 * total))
        return busy

    phase_rows = []
    bounds = phases + [(float("inf"), "")]
    for (start, name), (end, _) in zip(bounds, bounds[1:]):
        win = [w for w in windows if start < w[0] <= end]
        if win and name != "采集结束":
            phase_rows.append((name, start, min(end, windows[-1][0]), load(win)))
    whole = load(windows)
    idle_phase = next((r[3] for r in phase_rows if r[0].startswith("阶段4")), None)
    peak = max(windows, key=lambda w: max(load([w])["core0"], load([w])["core1"]))
    peak_load = max(load([peak])["core0"], load([peak])["core1"])
    metrics.update(
        cpu_core0_load_pct=round(whole["core0"], 2), cpu_core1_load_pct=round(whole["core1"], 2),
        cpu_business_load_pct=round(whole["business"], 3),
        cpu_idle_phase_core0_pct=round(idle_phase["core0"], 2) if idle_phase else None,
        cpu_idle_phase_core1_pct=round(idle_phase["core1"], 2) if idle_phase else None,
        cpu_idle_phase_business_pct=round(idle_phase["business"], 3) if idle_phase else None,
        cpu_peak_window_load_pct=round(peak_load, 1), cpu_unaccounted_pct=round(whole["unaccounted"], 2),
        cpu_probe_pct=round(whole["probe"], 3), cpu_mhz=hello.get("cpu_mhz"),
    )
    task_cpu = defaultdict(int)
    for w in windows:
        for n, d in w[2].items():
            task_cpu[n] += d
    total_us = sum(w[1] for w in windows)

    # 事件
    by_name = defaultdict(list)
    for s in spans:
        by_name[s["name"]].append(s)

    def span_stats(name):
        items = by_name.get(name, [])
        durs = [s["dur_us"] / 1000 for s in items]
        return len(items), (statistics.mean(durs) if durs else 0), (max(durs) if durs else 0)

    for name in ("screen_full", "screen_full_bydata", "screen_full_partial", "screen_refresh", "http_perform", "light_sleep"):
        n, avg, mx = span_stats(name)
        metrics[f"{name}_count"] = n
        metrics[f"{name}_avg_ms"] = round(avg, 1)
        metrics[f"{name}_max_ms"] = round(mx, 1)
    metrics["http_errors"] = sum(1 for s in by_name.get("http_perform", []) if s["arg"] != 0)
    metrics["ap_sta_join_count"] = len(by_name.get("ap_sta_join", []))
    # 自动剧本：探针模拟的操作（arg=1 表示走了触摸模拟，0 表示退回直接发 UI 事件），以及电脑或回环发起的配网页请求
    auto = hello.get("auto", 0) == 1
    taps = by_name.get("auto_tap", []) + by_name.get("auto_swipe", [])
    host_http = [dict(re.findall(r"(\w+)=(\S+)", l)) for l in lines if l.startswith("HOST http ")]
    loop_http = by_name.get("auto_http", [])
    http_ok = [h for h in host_http if h.get("status") == "200"] + [h for h in loop_http if h["arg"] > 0]
    metrics.update(
        auto_scenario=auto, auto_touch_count=len(taps), auto_touch_simulated=sum(1 for t in taps if t["arg"] == 1),
        auto_unsupported=len(by_name.get("auto_unsupported", [])),
        config_page_requests=len(host_http) + len(loop_http), config_page_requests_ok=len(http_ok),
        config_page_request_mode="host" if host_http else ("loopback" if loop_http else "none"),
    )
    if host_http:
        metrics["config_page_avg_ms"] = round(statistics.mean(float(h["ms"]) for h in host_http if h.get("status") == "200") if http_ok else 0, 1)
    elif loop_http:
        metrics["config_page_avg_ms"] = round(statistics.mean(h["dur_us"] / 1000 for h in loop_http), 1)
    metrics.update(crashes=crashes, reboots=reboots, spans_dropped=heaps[-1].get("spans_dropped", 0),
                   serial_dropped_groups=sum(dropped.values()), probe_version=hello.get("version", 1))
    if spiffs:
        metrics.update(spiffs_total_bytes=int(spiffs.group(1)), spiffs_used_bytes=int(spiffs.group(2)))

    capture_s = windows[-1][0] / 1000
    L = ["# 设备实测", "", f"- 被测代码：{source_desc}", f"- 设备上的固件版本：{fw.group(1) if fw else '未知'}（测量固件 = 被测代码 + perf_probe）",
         f"- 采集时长约 {capture_s:.0f} s，采样周期 {hello.get('sample_ms')} ms，CPU {hello.get('cpu_mhz')} MHz",
         (f"- 剧本自动执行：模拟操作 {metrics['auto_touch_count']} 次（其中走触摸模拟 {metrics['auto_touch_simulated']} 次），"
          f"配网页请求 {metrics['config_page_requests_ok']}/{metrics['config_page_requests']} 次成功（方式：{metrics['config_page_request_mode']}）"
          if auto else "- 剧本人工执行") +
         f"；刷屏 {sum(len(by_name[n]) for n in by_name if n.startswith('screen_'))} 次，"
         f"AP 客户端接入 {metrics['ap_sta_join_count']} 次，天气等 HTTP 请求 {metrics['http_perform_count']} 次",
         f"- 异常：崩溃 {crashes} 次，额外重启 {reboots} 次，探针丢弃事件 {metrics['spans_dropped']} 条",
         f"- 串口丢数据：丢弃不完整的采样组 {dropped['tasks']} 组、堆记录 {dropped['heap']} 条、事件 {dropped['span']} 条"
         f"（共采用 {len(samples)} 组任务采样），探针版本 {metrics['probe_version']}", ""]

    L += ["## 启动时间线", "", "时间以 esp_timer 启动为起点（约比芯片复位晚 0.5 秒，bootloader 加载 app 的时间不含在内）。", "",
          "| 事件 | 时间 |", "|---|---|"]
    names = {"boot_app_main_ms": "进入 app_main", "boot_got_ip_ms": "STA 拿到 IP", "boot_time_valid_ms": "系统时间有效（对时完成）",
             "boot_first_screen_done_ms": "第一次刷屏完成", "boot_first_http_done_ms": "第一次 HTTP 请求完成"}
    L += [f"| {names[k]} | {ms(v)} |" for k, v in boot.items()]

    L += ["", "## 堆（MALLOC_CAP_8BIT）", "", "| 指标 | 字节 |", "|---|---|"]
    heap_names = {"heap_total8": "堆总量", "heap_free_boot": "进入 app_main 时剩余", "heap_free_idle_median": "空闲阶段剩余（中位数）",
                  "heap_min8": "历史最低剩余", "heap_int_min": "内部 RAM 历史最低剩余", "heap_largest_min": "最大连续空闲块（最小值）",
                  "probe_overhead_bytes": "探针自身开销（静态 + 任务栈/缓冲）", "heap_min8_without_probe": "历史最低剩余（扣除探针开销，估算）"}
    L += [f"| {heap_names[k]} | {metrics[k]} |" for k in heap_names]

    L += ["", "## 任务栈（采样期间最小剩余）", "", f"剩余低于 {STACK_RISK_BYTES} 字节标 ⚠。栈大小为 0 表示没捕获到创建参数。", "",
          "| 任务 | 优先级 | 核 | 栈 | 最小剩余 | 已用 | 占比 |", "|---|---|---|---|---|---|---|"]
    for name, (hwm, stack, prio, core) in sorted(stacks.items(), key=lambda kv_: kv_[1][0]):
        used = stack - hwm if stack else None
        pct = f"{100 * used / stack:.0f}%" if stack else "-"
        flag = " ⚠" if hwm < STACK_RISK_BYTES else ""
        L.append(f"| {name}{' （探针）' if name == 'perf_probe' else ''} | {prio} | {core if core >= 0 else '不绑'} | {stack or '?'} | "
                 f"{hwm}{flag} | {used if used is not None else '-'} | {pct} |")

    probe_note = f"探针固定在 core{probe_core}，该核负载已扣除探针时间" if probe_core >= 0 else "探针没有绑核（第 1 版），两个核的负载里都可能混有探针时间，以业务负载为准"
    L += ["", "## CPU 占用", "",
          "按测试剧本分阶段统计。核负载 = 1 − IDLE 任务占比；业务负载 = 除 IDLE 和探针以外所有任务的时间 / 两核总时间；"
          f"未计入 = 两核时间中没有记到任何任务上的部分（light sleep 期间）。{probe_note}。中断时间算在被打断的任务上。", "",
          "| 阶段 | 时间段 | core0 负载 | core1 负载 | 业务负载（两核合计） | 探针 | 未计入 |", "|---|---|---|---|---|---|---|"]
    for name, start, end, ld in phase_rows + [("全程", 0, windows[-1][0], whole)]:
        L.append(f"| {name} | {start / 1000:.0f}–{end / 1000:.0f} s | {ld['core0']:.2f}% | {ld['core1']:.2f}% | {ld['business']:.3f}% | "
                 f"{ld['probe']:.3f}% | {ld['unaccounted']:.2f}% |")
    L += ["", f"单个采样窗口（{hello.get('sample_ms')} ms）里单核负载的峰值：{peak_load:.1f}%（窗口结束于 {peak[0] / 1000:.0f} s）。", "",
          "### 各任务 CPU 时间（全程，占两核总时间）", "", "| 任务 | CPU 时间 | 占比 |", "|---|---|---|"]
    for n, d in sorted(task_cpu.items(), key=lambda kv_: -kv_[1])[:15]:
        L.append(f"| {n} | {d / 1000:.0f} ms | {100 * d / (2 * total_us):.3f}% |")

    L += ["", "## 事件", "", "| 事件 | 次数 | 平均耗时 | 最长耗时 |", "|---|---|---|---|"]
    ev_names = {"screen_full": "墨水屏全刷（天气、单词页）", "screen_full_bydata": "墨水屏全刷（主页、设置页整屏位图）",
                "screen_full_partial": "墨水屏全刷（时间、番茄页拼图后整页刷新）",
                "screen_refresh": "墨水屏局刷（跨文件调用的 ds_screen_refresh）", "http_perform": "HTTP 请求", "light_sleep": "light sleep"}
    for k, label in ev_names.items():
        L.append(f"| {label} | {metrics[f'{k}_count']} | {metrics[f'{k}_avg_ms']} ms | {metrics[f'{k}_max_ms']} ms |")
    if metrics.get("config_page_avg_ms") is not None:
        L.append(f"| 配网页请求（{metrics['config_page_request_mode']}，含 / 和 /favicon.ico） | {metrics['config_page_requests']} | "
                 f"{metrics['config_page_avg_ms']} ms | - |")
    if spiffs:
        L += ["", f"SPIFFS：总量 {spiffs.group(1)} 字节，已用 {spiffs.group(2)} 字节。"]
    L.append("")

    (out / "summary.md").write_text("\n".join(L), encoding="utf-8")
    (out / "metrics.json").write_text(json.dumps({"source": source_desc, "device": metrics}, ensure_ascii=False, indent=2),
                                      encoding="utf-8")


if __name__ == "__main__":
    main()

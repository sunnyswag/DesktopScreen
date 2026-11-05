"""对比两次评估结果：python3 tools/perf/compare.py <基线结果目录> <新结果目录> [> 对比.md]

结果目录就是 size_report.sh / code_metrics.sh / device_probe.sh 的 --out 目录，
读取其中 size/、code/、device/ 下的 metrics.json，输出 Markdown 对比表。
"""
import json
import sys
from pathlib import Path

# 数值越大越好的指标；其余默认越小越好，None 表示只看变化不判断好坏
HIGHER_IS_BETTER = {
    "app_free_bytes", "bootloader_free_bytes", "flash_unallocated_bytes",
    "heap_total8", "heap_free_boot", "heap_free_idle_median", "heap_min8", "heap_int_min", "heap_largest_min",
    "heap_min8_without_probe", "stack_min_margin_bytes", "config_page_requests_ok", "auto_touch_simulated",
}
NEUTRAL = {
    "app_partition_bytes", "iram_total_bytes", "dram_total_bytes", "functions", "source_files", "tasks_count",
    "screen_full_count", "screen_full_bydata_count", "screen_refresh_count", "http_perform_count", "light_sleep_count",
    "ap_sta_join_count", "cpu_mhz", "probe_overhead_bytes", "spiffs_total_bytes", "comment_lines",
    "stack_total_alloc_bytes", "light_sleep_avg_ms", "light_sleep_max_ms", "screen_full_partial_count",
    "auto_scenario", "auto_touch_count", "config_page_requests", "probe_version", "boot_time_valid_ms",
}


def load(run):
    data = {}
    for part in ("size", "code", "device"):
        path = Path(run) / part / "metrics.json"
        if path.exists():
            j = json.load(open(path))
            data[part] = (j.get("source", ""), j[part])
    return data


def verdict(key, old, new):
    numeric = lambda v: isinstance(v, (int, float)) and not isinstance(v, bool)
    if not numeric(old) or not numeric(new) or old == new or key in NEUTRAL:
        return ""
    better = new > old if key in HIGHER_IS_BETTER else new < old
    return "✅" if better else "⚠️"


def main():
    base, new = load(sys.argv[1]), load(sys.argv[2])
    titles = {"size": "静态资源（size_report.sh）", "code": "代码量与复杂度（code_metrics.sh）", "device": "设备实测（device_probe.sh）"}
    print("# 评估结果对比\n")
    for part, title in titles.items():
        if part not in base and part not in new:
            continue
        b_src, b = base.get(part, ("（缺）", {}))
        n_src, n = new.get(part, ("（缺）", {}))
        print(f"## {title}\n\n- 基线：{b_src}\n- 新版：{n_src}\n")
        print("| 指标 | 基线 | 新版 | 变化 | |\n|---|---|---|---|---|")
        for key in list(b) + [k for k in n if k not in b]:
            old, cur = b.get(key), n.get(key)
            delta = ""
            if all(isinstance(v, (int, float)) and not isinstance(v, bool) for v in (old, cur)):
                d = cur - old
                delta = f"{d:+.2f}" if isinstance(d, float) else f"{d:+d}"
                if old:
                    delta += f"（{100 * d / old:+.1f}%）"
            print(f"| {key} | {old if old is not None else '-'} | {cur if cur is not None else '-'} | {delta} | {verdict(key, old, cur)} |")
        print()


if __name__ == "__main__":
    main()

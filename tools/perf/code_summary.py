"""code_metrics.sh 的汇总脚本：code_summary.py <结果目录> <被测代码描述>

文件级行数自己统计（去掉注释和空行后的行数记为 NLOC），函数级 NLOC/CCN 取 lizard 的结果。
"""
import csv
import json
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path

CCN_WARN = 10
CCN_HIGH = 15
LONG_FUNC_NLOC = 60


def strip_comments(text):
    """去掉 C 注释，保留字符串和换行，粗略但对本项目够用。"""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text.count("\n", i, j))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def module_of(rel):
    parts = rel.split("/")
    if parts[1] == "include":
        return "include"
    if parts[1] == "src" and len(parts) > 3:
        return f"src/{parts[2]}"
    return "/".join(parts[1:])


def main():
    out, source_desc = Path(sys.argv[1]), sys.argv[2]
    src = out / "src"
    files = [src / "main/app_main.c"] + sorted(p for p in (src / "main/src").rglob("*.[ch]") if "/src/data/" not in str(p))
    files += sorted((src / "main/include").rglob("*.h"))
    data_files = sorted((src / "main/src/data").glob("*.c"))

    per_module = defaultdict(lambda: defaultdict(int))
    totals = defaultdict(int)
    for path in files:
        rel = str(path.relative_to(src))
        raw = path.read_text(encoding="utf-8", errors="replace")
        code = strip_comments(raw)
        stats = {
            "files": 1,
            "lines": raw.count("\n") + (0 if raw.endswith("\n") else 1),
            "nloc": sum(1 for line in code.splitlines() if line.strip()),
            "blank": sum(1 for line in raw.splitlines() if not line.strip()),
            "printf": len(re.findall(r"\bprintf\s*\(", code)),
            "esp_log": len(re.findall(r"\bESP_LOG[EWIDV]\s*\(", code)),
        }
        stats["comment"] = stats["lines"] - stats["nloc"] - stats["blank"]
        for k, v in stats.items():
            per_module[module_of(rel)][k] += v
            totals[k] += v

    funcs = []
    for row in csv.reader(open(out / "lizard.csv", encoding="utf-8")):
        nloc, ccn, token, param, length = (int(x) for x in row[:5])
        funcs.append({"nloc": nloc, "ccn": ccn, "param": param, "file": row[6], "name": row[7], "line": int(row[9])})
    ccns = [f["ccn"] for f in funcs]

    dup_text = (out / "lizard-duplicate.txt").read_text(errors="replace")
    dup_rate = re.search(r"Total duplicate rate: ([\d.]+)%", dup_text)
    dup_blocks = re.findall(r"Duplicate block:\n-+\n(.*?)\^\^\^", dup_text, re.S)

    data_lines = sum(p.read_text(errors="replace").count("\n") for p in data_files)
    data_bytes = sum(p.stat().st_size for p in data_files)

    metrics = {
        "source_files": totals["files"],
        "physical_lines": totals["lines"],
        "nloc": totals["nloc"],
        "comment_lines": totals["comment"],
        "functions": len(funcs),
        "avg_ccn": round(statistics.mean(ccns), 2),
        "max_ccn": max(ccns),
        "funcs_ccn_gt_10": sum(c > CCN_WARN for c in ccns),
        "funcs_ccn_gt_15": sum(c > CCN_HIGH for c in ccns),
        "avg_func_nloc": round(statistics.mean(f["nloc"] for f in funcs), 1),
        "max_func_nloc": max(f["nloc"] for f in funcs),
        "funcs_nloc_gt_60": sum(f["nloc"] > LONG_FUNC_NLOC for f in funcs),
        "duplicate_rate_pct": float(dup_rate.group(1)) if dup_rate else 0.0,
        "duplicate_blocks": len(dup_blocks),
        "printf_calls": totals["printf"],
        "esp_log_calls": totals["esp_log"],
        "data_dir_files": len(data_files),
        "data_dir_lines": data_lines,
        "data_dir_bytes": data_bytes,
    }

    lines = ["# 代码量与复杂度", "", f"- 被测代码：{source_desc}",
             f"- 范围：main/app_main.c、main/src（不含 data）、main/include；工具 lizard（-l c）", "",
             "## 总览", "", "| 指标 | 数值 |", "|---|---|"]
    labels = {
        "source_files": "源文件数（.c/.h）", "physical_lines": "物理行", "nloc": "代码行 NLOC（去注释和空行）",
        "comment_lines": "注释行", "functions": "函数数", "avg_ccn": "平均圈复杂度", "max_ccn": "最大圈复杂度",
        "funcs_ccn_gt_10": f"CCN > {CCN_WARN} 的函数", "funcs_ccn_gt_15": f"CCN > {CCN_HIGH} 的函数",
        "avg_func_nloc": "平均函数长度（NLOC）", "max_func_nloc": "最长函数（NLOC）",
        "funcs_nloc_gt_60": f"长度 > {LONG_FUNC_NLOC} NLOC 的函数", "duplicate_rate_pct": "重复代码率（%）",
        "duplicate_blocks": "重复代码块", "printf_calls": "printf 调用", "esp_log_calls": "ESP_LOGx 调用",
        "data_dir_files": "data 目录文件数", "data_dir_lines": "data 目录行数", "data_dir_bytes": "data 目录源码字节",
    }
    lines += [f"| {labels[k]} | {v} |" for k, v in metrics.items()]

    lines += ["", "## 按模块", "", "| 模块 | 文件 | 物理行 | NLOC | 注释行 | printf | ESP_LOGx |", "|---|---|---|---|---|---|---|"]
    for mod in sorted(per_module):
        s = per_module[mod]
        lines.append(f"| {mod} | {s['files']} | {s['lines']} | {s['nloc']} | {s['comment']} | {s['printf']} | {s['esp_log']} |")

    lines += ["", f"## 圈复杂度最高的函数", "", "| 函数 | 位置 | CCN | NLOC | 参数 |", "|---|---|---|---|---|"]
    for f in sorted(funcs, key=lambda f: (-f["ccn"], -f["nloc"]))[:12]:
        lines.append(f"| {f['name']} | {f['file']}:{f['line']} | {f['ccn']} | {f['nloc']} | {f['param']} |")
    lines += ["", f"## 最长的函数", "", "| 函数 | 位置 | NLOC | CCN |", "|---|---|---|---|"]
    for f in sorted(funcs, key=lambda f: -f["nloc"])[:8]:
        lines.append(f"| {f['name']} | {f['file']}:{f['line']} | {f['nloc']} | {f['ccn']} |")
    lines += ["", "## 重复代码块（lizard -Eduplicate）", ""]
    lines += [f"- {' ↔ '.join(x.strip() for x in block.strip().splitlines())}" for block in dup_blocks] or ["- 无"]
    lines.append("")

    (out / "summary.md").write_text("\n".join(lines), encoding="utf-8")
    (out / "metrics.json").write_text(json.dumps({"source": source_desc, "code": metrics}, ensure_ascii=False, indent=2),
                                      encoding="utf-8")


if __name__ == "__main__":
    main()

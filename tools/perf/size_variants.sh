#!/usr/bin/env bash
# tools/perf/size_variants.sh — sdkconfig 对照实验：每组只改配置，编译后和 base 比较 app 体积和 RAM 段
#
# 用法：bash tools/perf/size_variants.sh [--ref <git-ref> | --worktree] [--out <目录>] [--jobs 2]
# 对照组定义在 variants.txt。每组一次全量编译，默认两组并行，7 组约 10~15 分钟。
set -euo pipefail
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
JOBS=2
perf_parse_args "$@"
set -- "${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}"
while [ $# -gt 0 ]; do
  case "$1" in
    --jobs) JOBS=$2; shift 2 ;;
    *) die "未知参数 $1" ;;
  esac
done

OUT=$OUT_DIR/variants
mkdir -p "$OUT"
log "导出被测代码：$PERF_SOURCE_DESC"
perf_prepare_source "$OUT/src"
perf_load_idf

build_variant() {
  local name=$1; shift
  local dir=$OUT/$name
  rm -rf "$dir"
  cp -r "$OUT/src" "$dir"
  [ $# -gt 0 ] && perf_set_config "$dir/sdkconfig" "$@"
  if idf.py -C "$dir" -B "$dir/build" build >"$dir/build.log" 2>&1; then
    local map
    map=$(ls "$dir"/build/*.map | head -1)
    (cd "$dir" && ESP_IDF_SIZE_NG=1 python -m esp_idf_size --format json2 "$map" >size.json)
    log "$name 完成"
  else
    log "$name 编译失败，日志：$dir/build.log"
  fi
}

mapfile -t VARIANTS < <(grep -v -e '^#' -e '^\s*$' "$PERF_DIR/variants.txt")
running=0
for line in "${VARIANTS[@]}"; do
  # shellcheck disable=SC2086
  build_variant $line &
  running=$((running + 1))
  if [ "$running" -ge "$JOBS" ]; then
    wait -n || true
    running=$((running - 1))
  fi
done
wait

python3 -I - "$OUT" "$PERF_DIR/variants.txt" "$PERF_SOURCE_DESC" <<'EOF'
import json, re, sys
from pathlib import Path
out, variants, desc = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]

def load(name):
    d = out / name
    log = (d / "build.log").read_text(errors="replace") if (d / "build.log").exists() else ""
    m = re.search(r"binary size (0x[0-9a-f]+) bytes\. Smallest app partition is (0x[0-9a-f]+)", log)
    if not m or not (d / "size.json").exists():
        return None
    layout = {s["name"]: s for s in json.load(open(d / "size.json"))["layout"]}
    return {"bin": int(m.group(1), 16), "part": int(m.group(2), 16),
            "iram": layout["IRAM"]["used"], "dram": layout["DRAM"]["used"]}

rows = [l.split(None, 1) for l in variants.read_text().splitlines() if l.strip() and not l.startswith("#")]
base = load("base")
lines = ["# sdkconfig 对照实验", "", f"- 被测代码：{desc}", "",
         "| 组 | 改动 | app.bin | 相对 base | 分区剩余 | IRAM 变化 | DRAM 变化 |", "|---|---|---|---|---|---|---|"]
metrics = {}
for row in rows:
    name, settings = row[0], (row[1] if len(row) > 1 else "")
    r = load(name)
    if r is None:
        lines.append(f"| {name} | {settings or '-'} | 编译失败 | | | | |")
        continue
    metrics[name] = r
    free = r["part"] - r["bin"]
    lines.append(f"| {name} | `{settings}` | {r['bin']} | {r['bin'] - base['bin']:+d} | {free}（{100 * free / r['part']:.0f}%） | "
                 f"{r['iram'] - base['iram']:+d} | {r['dram'] - base['dram']:+d} |" if settings else
                 f"| {name} | - | {r['bin']} | 0 | {free}（{100 * free / r['part']:.0f}%） | 0 | 0 |")
(out / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
(out / "metrics.json").write_text(json.dumps({"source": desc, "variants": metrics}, ensure_ascii=False, indent=2), encoding="utf-8")
EOF
log "完成：$OUT/summary.md"

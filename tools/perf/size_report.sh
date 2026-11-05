#!/usr/bin/env bash
# tools/perf/size_report.sh — 静态资源评估：Flash/RAM 各段占用、分区余量、组件排行、data 目录占用、编译告警
#
# 用法：bash tools/perf/size_report.sh [--ref <git-ref> | --worktree] [--out <目录>]
# 输出：<out>/size/summary.md、metrics.json，以及 esp_idf_size 的原始输出和完整编译日志
set -euo pipefail
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
perf_parse_args "$@"

OUT=$OUT_DIR/size
SRC=$OUT/src
mkdir -p "$OUT"
log "导出被测代码：$PERF_SOURCE_DESC"
perf_prepare_source "$SRC"
perf_load_idf

log "全量编译（约 3~5 分钟）"
idf.py -C "$SRC" -B "$SRC/build" build >"$OUT/build.log" 2>&1 || { tail -40 "$OUT/build.log"; die "编译失败，日志：$OUT/build.log"; }

ELF=$(ls "$SRC"/build/*.elf | head -1)
MAP=${ELF%.elf}.map
cd "$OUT"
for mode in "" --archives --files; do
  name=size${mode:+-${mode#--}}
  ESP_IDF_SIZE_NG=1 python -m esp_idf_size $mode "$MAP" >"$name.txt"
  ESP_IDF_SIZE_NG=1 python -m esp_idf_size $mode --format json2 "$MAP" >"$name.json"
done
xtensa-esp32-elf-nm -S -t d "$ELF" | awk '$4 ~ /^gImage_/' >data-symbols.txt || true
cp "$SRC/partitions.csv" partitions.csv

python3 -I "$PERF_DIR/size_summary.py" "$OUT" "$PERF_SOURCE_DESC"
log "完成：$OUT/summary.md"

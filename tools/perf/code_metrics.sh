#!/usr/bin/env bash
# tools/perf/code_metrics.sh — 代码量与复杂度：NLOC、函数数、圈复杂度（CCN）、长函数、重复代码、日志调用分布
#
# 用法：bash tools/perf/code_metrics.sh [--ref <git-ref> | --worktree] [--out <目录>]
# 依赖：lizard（固定版本，首次运行自动装到 ~/.cache/desktopscreen-perf/venv）
# 统计范围：main/app_main.c、main/src、main/include；main/src/data（位图数组）单独计数，不计入 NLOC 和复杂度
set -euo pipefail
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"
perf_parse_args "$@"

LIZARD_VERSION=1.17.13
OUT=$OUT_DIR/code
SRC=$OUT/src
mkdir -p "$OUT"
log "导出被测代码：$PERF_SOURCE_DESC"
perf_prepare_source "$SRC"

if ! "$PERF_VENV/bin/lizard" --version 2>/dev/null | grep -qx "$LIZARD_VERSION"; then
  log "安装 lizard $LIZARD_VERSION 到 $PERF_VENV"
  python3 -m venv "$PERF_VENV"
  "$PERF_VENV/bin/pip" install -q "lizard==$LIZARD_VERSION"
fi

cd "$SRC"
TARGETS=(main/app_main.c main/src main/include)
"$PERF_VENV/bin/lizard" -l c --csv "${TARGETS[@]}" -x "*/src/data/*" >"$OUT/lizard.csv"
"$PERF_VENV/bin/lizard" -l c -Eduplicate "${TARGETS[@]}" -x "*/src/data/*" >"$OUT/lizard-duplicate.txt" || true

python3 -I "$PERF_DIR/code_summary.py" "$OUT" "$PERF_SOURCE_DESC"
log "完成：$OUT/summary.md"

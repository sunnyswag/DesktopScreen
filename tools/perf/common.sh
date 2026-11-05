# tools/perf/common.sh — perf 脚本的公共函数，只能被同目录的脚本 source
#
# 公共参数：
#   --ref <git-ref>   评估某个提交（默认 HEAD），用 git archive 导出，不含未提交改动
#   --worktree        评估当前工作区（包含未提交和未跟踪的文件）
#   --out <目录>      结果目录，默认 perf-out/<日期>-<短哈希>[-wt]/（已在 .gitignore 里忽略）
# 被测代码总是先复制到结果目录下的 src/ 再编译，不会动仓库里的 build/ 和 sdkconfig。

PERF_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_DIR=$(cd "$PERF_DIR/../.." && pwd)
PERF_VENV=${XDG_CACHE_HOME:-$HOME/.cache}/desktopscreen-perf/venv

die() { echo "错误：$*" >&2; exit 1; }
log() { echo "[perf] $*" >&2; }

SRC_MODE=ref
SRC_REF=HEAD
OUT_DIR=
EXTRA_ARGS=()

perf_parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --ref) SRC_MODE=ref; SRC_REF=$2; shift 2 ;;
      --worktree) SRC_MODE=worktree; shift ;;
      --out) OUT_DIR=$2; shift 2 ;;
      *) EXTRA_ARGS+=("$1"); shift ;;
    esac
  done
  local sha
  sha=$(git -C "$REPO_DIR" rev-parse --short "$SRC_REF") || die "找不到提交 $SRC_REF"
  if [ "$SRC_MODE" = worktree ]; then
    PERF_LABEL="$(date +%F)-${sha}-wt"
    PERF_SOURCE_DESC="工作区（基于 ${sha}，含未提交改动）"
  else
    PERF_LABEL="$(date +%F)-${sha}"
    PERF_SOURCE_DESC="提交 $(git -C "$REPO_DIR" rev-parse "$SRC_REF")"
  fi
  OUT_DIR=${OUT_DIR:-$REPO_DIR/perf-out/$PERF_LABEL}
  mkdir -p "$OUT_DIR"
  OUT_DIR=$(cd "$OUT_DIR" && pwd)
}

# 把被测代码复制到 $1（会先清空）
perf_prepare_source() {
  local dest=$1
  rm -rf "$dest"
  mkdir -p "$dest"
  if [ "$SRC_MODE" = worktree ]; then
    (cd "$REPO_DIR" && git ls-files -z --cached --others --exclude-standard |
      tar --null --ignore-failed-read -T - -cf - 2>/dev/null) | tar -xf - -C "$dest"
  else
    git -C "$REPO_DIR" archive "$SRC_REF" | tar -x -C "$dest"
  fi
}

perf_load_idf() {
  if ! command -v idf.py >/dev/null 2>&1; then
    # shellcheck disable=SC1091
    . "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh" >/dev/null 2>&1 || die "无法加载 ESP-IDF 环境"
  fi
}

# 修改 sdkconfig：perf_set_config <sdkconfig> KEY=VALUE...（VALUE 为 n 表示 "is not set"）
perf_set_config() {
  python3 -I "$PERF_DIR/setcfg.py" "$@"
}

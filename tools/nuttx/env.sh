# tools/env.sh — DesktopScreen NuttX 开发环境（只能 source，不能直接执行）
# TODO（阶段 B）：NuttX 改为用 repo manifest 拉到 vendor/ 下后，NUTTX_DIR/APPS_DIR 默认值要改成 vendor 路径。
#
# 用法：source tools/env.sh
# 首次使用请先运行 tools/setup.sh

# 被直接执行时提示并退出（不会影响用户 shell）
# bash 用 BASH_SOURCE 判断；zsh 没有 BASH_SOURCE，且 source 时 $0 就是脚本路径，要看 ZSH_EVAL_CONTEXT
if [ -n "${ZSH_VERSION:-}" ]; then
  case "$ZSH_EVAL_CONTEXT" in
    *:file*) ;;
    *) echo "本脚本只能 source 使用：source tools/env.sh" >&2; exit 1 ;;
  esac
  eval '_env_self=${(%):-%x}'
elif [ -n "${BASH_VERSION:-}" ]; then
  if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    echo "本脚本只能 source 使用：source tools/env.sh" >&2
    exit 1
  fi
  _env_self=${BASH_SOURCE[0]}
else
  echo "请在 bash 或 zsh 中 source 本脚本" >&2
  return 1 2>/dev/null || exit 1
fi

# ---------- 默认配置（可预先用环境变量覆盖，与 tools/setup.sh 保持一致） ----------
NUTTX_TAG="${NUTTX_TAG:-nuttx-12.10.0}"
NUTTX_DIR="${NUTTX_DIR:-$HOME/Git/nuttx}"
APPS_DIR="${APPS_DIR:-$HOME/Git/apps}"
VENV_DIR="${VENV_DIR:-$HOME/.venvs/nuttx}"
DS_TOOLCHAIN_BIN="${DS_TOOLCHAIN_BIN:-$HOME/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin}"
# 字库路径与烧写偏移：0x290000 是 docs/PLAN.md 建议的新 Flash 布局（阶段 C 定稿），
# 不要用原项目的 0x15D000
DS_FONT="${DS_FONT:-$HOME/esp/DesktopScreen/myFont1.bin}"
DS_FONT_OFFSET="${DS_FONT_OFFSET:-0x290000}"

# 仓库根目录（按脚本自身位置定位，支持在任意当前目录 source）
DS_REPO=$(cd -- "$(dirname -- "$_env_self")/../.." && pwd)
unset _env_self

export DS_REPO NUTTX_TAG NUTTX_DIR APPS_DIR VENV_DIR
export DS_TOOLCHAIN_BIN DS_FONT DS_FONT_OFFSET

# ---------- 工具链 PATH（已在 PATH 里就不重复加） ----------
if [ -d "$DS_TOOLCHAIN_BIN" ]; then
  case ":$PATH:" in
    *":$DS_TOOLCHAIN_BIN:"*) ;;
    *) export PATH="$DS_TOOLCHAIN_BIN:$PATH" ;;
  esac
else
  echo "提示：工具链目录不存在：$DS_TOOLCHAIN_BIN" >&2
fi

# ---------- 激活 venv ----------
_env_venv=未激活
if [ -f "$VENV_DIR/bin/activate" ]; then
  # shellcheck disable=SC1091
  . "$VENV_DIR/bin/activate"
  _env_venv=已激活
else
  echo "提示：venv 不存在（$VENV_DIR），请先运行 tools/setup.sh" >&2
fi

# ---------- 命令 ----------
# 配置 NuttX：ds_configure <board:config | 自定义板 config 目录路径>
# 已有 .config 时先 make distclean；-l 指定 Linux 主机环境；
# -a 的 apps 路径必须是相对 nuttx 目录的（tools/configure.sh:46-47、325），
# 自定义板直接传 config 目录的绝对路径（tools/configure.sh:52-54、157-172）
ds_configure() {
  if [ $# -ne 1 ]; then
    echo "用法：ds_configure <board:config | 自定义板 config 目录路径>" >&2
    return 1
  fi
  if [ ! -d "$NUTTX_DIR" ]; then
    echo "错误：NUTTX_DIR 不存在：$NUTTX_DIR（请先运行 tools/setup.sh）" >&2
    return 1
  fi
  local apps_rel
  apps_rel=$(realpath --relative-to="$NUTTX_DIR" "$APPS_DIR") || return 1
  (
    cd "$NUTTX_DIR" || exit 1
    if [ -f .config ]; then
      make distclean || exit 1
    fi
    ./tools/configure.sh -l -a "$apps_rel" "$1"
  )
}

# 编译 NuttX：ds_build [额外 make 参数...]
ds_build() {
  make -C "$NUTTX_DIR" -j"$(nproc)" "$@"
}

# 打开菜单配置：ds_menuconfig
ds_menuconfig() {
  make -C "$NUTTX_DIR" menuconfig
}

# 烧录固件（Simple Boot）：ds_flash [端口]
# ESPTOOL_PORT 必需（tools/esp32/Config.mk:294-299）；Simple Boot 下
# ESPTOOL_BINDIR 会被自动设为 .，无需传入（tools/esp32/Config.mk:131-137）
ds_flash() {
  make -C "$NUTTX_DIR" flash "ESPTOOL_PORT=${1:-/dev/ttyUSB0}"
}

# 串口终端：ds_monitor [端口] [波特率]，Ctrl+] 退出
ds_monitor() {
  python -m serial.tools.miniterm "${1:-/dev/ttyUSB0}" "${2:-115200}"
}

# 把字库烧到 Flash 裸区：ds_flash_font [端口]
ds_flash_font() {
  if [ ! -f "$DS_FONT" ]; then
    echo "错误：字库文件不存在：$DS_FONT（可用 DS_FONT 变量覆盖）" >&2
    return 1
  fi
  local espcmd
  if command -v esptool.py >/dev/null 2>&1; then
    espcmd=esptool.py
  elif command -v esptool >/dev/null 2>&1; then
    espcmd=esptool
  else
    echo "错误：找不到 esptool，请先运行 tools/setup.sh 并重新 source 本脚本" >&2
    return 1
  fi
  "$espcmd" --chip esp32 --port "${1:-/dev/ttyUSB0}" --baud 460800 \
    write_flash "$DS_FONT_OFFSET" "$DS_FONT"
}

# 显示命令列表：ds_help
ds_help() {
  cat <<'EOF'
可用命令：
  ds_configure <board:config | config目录路径>   配置 NuttX（已有 .config 会先 distclean）
  ds_build [make参数...]                        编译（并行 make）
  ds_menuconfig                                 菜单配置
  ds_flash [端口]                               烧录固件（默认 /dev/ttyUSB0）
  ds_monitor [端口] [波特率]                     串口终端（默认 /dev/ttyUSB0 115200，Ctrl+] 退出）
  ds_flash_font [端口]                          烧写字库（偏移见 DS_FONT_OFFSET，默认 0x290000）
  ds_help                                       显示本帮助
EOF
}

# ---------- 摘要 ----------
_env_gcc=$("$DS_TOOLCHAIN_BIN/xtensa-esp32-elf-gcc" -dumpversion 2>/dev/null) || _env_gcc=未找到
echo "环境就绪：NuttX tag=$NUTTX_TAG  工具链=xtensa-esp32-elf-gcc $_env_gcc  venv=$_env_venv"
echo "输入 ds_help 查看可用命令。"
unset _env_venv _env_gcc

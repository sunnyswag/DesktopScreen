#!/usr/bin/env bash
# TODO（阶段 B）：NuttX 改为用 repo manifest 拉到 vendor/ 下后，本脚本要按新方式重写；当前仍是旧方案（clone 到 ~/Git/nuttx、~/Git/apps）。
# tools/setup.sh — DesktopScreen NuttX 移植环境的一次性准备（可重复运行，幂等）
#
# 做这些事：
#   1. 检查依赖命令与 ESP32 工具链（缺了只报错，不自动安装）
#   2. 准备 NuttX / nuttx-apps 源码：不存在就按 tag clone；存在则切到 tag（绝不 reset/stash）
#   3. 创建 Python venv，安装 esptool / kconfiglib / pyserial
#   4. 应用 patches/nuttx、patches/apps 下的补丁（已应用过的自动跳过）
#   5. 把仓库 apps/ 软链接到 nuttx-apps 的 external/
#
# 可用环境变量覆盖：NUTTX_TAG NUTTX_DIR APPS_DIR VENV_DIR DS_TOOLCHAIN_BIN

set -euo pipefail

# ---------- 配置（可用环境变量覆盖） ----------
NUTTX_TAG="${NUTTX_TAG:-nuttx-12.10.0}"
NUTTX_DIR="${NUTTX_DIR:-$HOME/Git/nuttx}"
APPS_DIR="${APPS_DIR:-$HOME/Git/apps}"
VENV_DIR="${VENV_DIR:-$HOME/.venvs/nuttx}"
DS_TOOLCHAIN_BIN="${DS_TOOLCHAIN_BIN:-$HOME/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin}"

# 仓库根目录（按脚本自身位置定位，可在任意当前目录运行）
REPO_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)

step() { printf '\n==> %s\n' "$*"; }
die()  { echo "错误：$*" >&2; exit 1; }

# 判断 dir 里的改动是否恰好是 patch_dir 下全部补丁已应用的状态
# （用于让"补丁已打"的工作区通过干净性检查，保证重复运行幂等）
all_patches_applied() {
  local dir=$1 patch_dir=$2 p found=0
  for p in "$patch_dir"/*.patch; do
    [ -e "$p" ] || continue
    found=1
    git -C "$dir" apply --reverse --check "$p" 2>/dev/null || return 1
  done
  [ "$found" -eq 1 ] || return 1
}

# 准备一个源码仓库：不存在就 clone；存在则确认干净后切到 tag。
# 已跟踪文件有改动时报错退出（例外：改动全部来自本仓库补丁）；未跟踪文件只提醒不删除。
prepare_repo() {
  local dir=$1 url=$2 tag=$3 name=$4 patch_dir=$5

  if [ ! -e "$dir" ]; then
    step "clone $name（tag $tag）到 $dir"
    git clone --branch "$tag" "$url" "$dir"
    return 0
  fi
  [ -d "$dir/.git" ] || die "$dir 已存在但不是 git 仓库，请自行处理。"

  step "检查 $name（$dir）"

  local untracked
  untracked=$(git -C "$dir" status --porcelain --untracked-files=all | grep '^??' || true)
  if [ -n "$untracked" ]; then
    echo "提醒：$name 有未跟踪文件（不影响构建，脚本不会删除）："
    printf '%s\n' "$untracked" | sed 's/^/    /'
  fi

  if [ -n "$(git -C "$dir" status --porcelain --untracked-files=no)" ]; then
    if all_patches_applied "$dir" "$patch_dir"; then
      echo "工作区的改动来自本仓库已应用的补丁，视为正常，继续。"
    else
      die "$name 有已跟踪文件的改动。请自行处理后重新运行；本脚本不会 reset/stash。"
    fi
  fi

  if ! git -C "$dir" rev-parse -q --verify "refs/tags/$tag^{commit}" >/dev/null 2>&1; then
    echo "本地没有 tag $tag，先 git fetch --tags ..."
    git -C "$dir" fetch --tags
  fi
  git -C "$dir" checkout "$tag"
  echo "$name 已在 tag $tag。"
}

# 按文件名排序逐个应用补丁：已应用（reverse --check 通过）则跳过；
# 能干净应用（--check 通过）则应用；两者都不行则报错退出。
apply_patches() {
  local name=$1 dir=$2 patch_dir=$3 p any=0
  if [ ! -d "$patch_dir" ]; then
    echo "没有 $name 补丁目录（$patch_dir），跳过。"
    return 0
  fi
  for p in "$patch_dir"/*.patch; do
    [ -e "$p" ] || continue
    any=1
    if git -C "$dir" apply --reverse --check "$p" 2>/dev/null; then
      echo "已应用过，跳过：$(basename "$p")"
    elif git -C "$dir" apply --check "$p" 2>/dev/null; then
      git -C "$dir" apply "$p"
      echo "已应用：$(basename "$p")"
    else
      die "补丁既不是已应用状态、也无法干净地应用到 $name：$p"
    fi
  done
  [ "$any" -eq 1 ] || echo "没有 $name 补丁（*.patch），跳过。"
}

# ---------- 1. 依赖检查 ----------
step "检查依赖命令和 ESP32 工具链"

missing=()
for cmd in git make gcc python3 gperf bison flex genromfs xxd; do
  command -v "$cmd" >/dev/null 2>&1 || missing+=("$cmd")
done
if [ ! -x "$DS_TOOLCHAIN_BIN/xtensa-esp32-elf-gcc" ]; then
  missing+=("xtensa-esp32-elf-gcc（在 $DS_TOOLCHAIN_BIN 中未找到）")
fi
if [ "${#missing[@]}" -gt 0 ]; then
  echo "缺少以下依赖，请自行安装后重新运行（本脚本不会自动安装）：" >&2
  printf '  - %s\n' "${missing[@]}" >&2
  exit 1
fi
echo "依赖检查通过。"

# ---------- 2. NuttX / nuttx-apps 源码 ----------
prepare_repo "$NUTTX_DIR" https://github.com/apache/nuttx.git "$NUTTX_TAG" \
  "NuttX 源码" "$REPO_ROOT/patches/nuttx"
prepare_repo "$APPS_DIR" https://github.com/apache/nuttx-apps.git "$NUTTX_TAG" \
  "nuttx-apps" "$REPO_ROOT/patches/apps"

# ---------- 3. Python venv ----------
step "准备 Python 虚拟环境（$VENV_DIR）"
if [ ! -d "$VENV_DIR" ]; then
  python3 -m venv "$VENV_DIR"
  echo "已创建 venv。"
fi
# esptool 加 <5 上限：NuttX 12.10 的构建脚本直接调用 esptool.py 命令
# （tools/esp32/Config.mk:302），esptool 5.x 已把入口改名为 esptool
# 最低版本 4.8.0 见 tools/esp32/Config.mk:71 的 ESPTOOL_MIN_VERSION
# pyserial 是 esptool 的依赖，显式安装以保证 ds_monitor 可用
"$VENV_DIR/bin/pip" install "esptool>=4.8,<5" kconfiglib pyserial

# ---------- 4. 补丁 ----------
step "应用补丁"
apply_patches "NuttX" "$NUTTX_DIR" "$REPO_ROOT/patches/nuttx"
apply_patches "apps" "$APPS_DIR" "$REPO_ROOT/patches/apps"

# ---------- 5. out-of-tree 应用软链接 ----------
step "链接 out-of-tree 应用（apps/external）"
if [ -f "$REPO_ROOT/apps/Makefile" ]; then
  ext="$APPS_DIR/external"
  if [ -L "$ext" ]; then
    if [ "$(readlink "$ext")" = "$REPO_ROOT/apps" ]; then
      echo "apps/external 已指向本仓库 apps/，无需处理。"
    else
      echo "警告：$ext 已指向别处（$(readlink "$ext")），跳过，不修改。"
    fi
  elif [ -e "$ext" ]; then
    echo "警告：$ext 已存在且不是软链接，跳过，不修改。"
  else
    ln -s "$REPO_ROOT/apps" "$ext"
    echo "已创建：$ext -> $REPO_ROOT/apps"
  fi
else
  echo "应用尚未创建（$REPO_ROOT/apps/Makefile 不存在），跳过。"
fi

# ---------- 完成 ----------
step "完成"
echo "下一步："
echo "  source tools/env.sh"
echo "  ds_configure sim:nsh    # 先用模拟器验证环境"

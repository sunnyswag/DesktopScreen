#!/usr/bin/env bash
# tools/perf/device_probe.sh — 设备实测：运行时堆、任务栈、CPU 占用、启动时间线、刷屏/HTTP/睡眠事件
#
# 用法：bash tools/perf/device_probe.sh [--ref <git-ref> | --worktree] [--out <目录>]
#         [--port /dev/ttyUSB0] [--seconds 300] [--baud 230400] [--no-restore]
#         [--manual] [--http host|loopback]
#
# 剧本（scenario.txt）默认自动执行，不需要人操作：
#   - 翻页：探针模拟触摸芯片（中断、电平、I2C 坐标），走完整的触摸链路；
#   - 配网页：--http host（默认）由电脑通过局域网请求设备的 httpd，要求电脑和设备在同一网段；
#             --http loopback 由设备自己通过 127.0.0.1 请求。两种都不包含手机关联 AP 的过程。
#   --manual 关闭自动剧本，改为按终端提示人工操作。
#
# 流程：
#   1. 导出被测代码，追加 probe/sdkconfig.probe，把 probe/perf_probe 组件链接进去编译测量固件
#   2. 读设备分区表，确认和测量固件的 app 分区一致（不一致就停止，避免写错位置）
#   3. 分块备份设备当前的 app 分区；只写 app 分区，NVS、字库、SPIFFS 都不动
#   4. 复位并采集串口 --seconds 秒，期间自动执行 scenario.txt 的剧本（--manual 时在终端提示人工操作）
#   5. 写回备份并复位，核对启动日志里的 App version 和备份一致（--no-restore 跳过）
#   6. 解析日志，输出 summary.md 和 metrics.json
#
# 注意：
#   - 运行前关掉占用串口的 idf.py monitor。
#   - WSL + usbipd 下大块高速读取容易出 "Corrupt data"，所以默认 230400 波特率、每块 256 KB 并重试。
#   - 原始日志 probe.log 可能含 WiFi 密码（system_data.c 会打印），只留在 perf-out/，不要提交。
set -euo pipefail
. "$(dirname "${BASH_SOURCE[0]}")/common.sh"

PORT=/dev/ttyUSB0
SECONDS_TO_CAPTURE=300
BAUD=230400
RESTORE=1
AUTO=1
HTTP_MODE=host
perf_parse_args "$@"
set -- "${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}"
while [ $# -gt 0 ]; do
  case "$1" in
    --port) PORT=$2; shift 2 ;;
    --seconds) SECONDS_TO_CAPTURE=$2; shift 2 ;;
    --baud) BAUD=$2; shift 2 ;;
    --no-restore) RESTORE=0; shift ;;
    --manual) AUTO=0; shift ;;
    --http) HTTP_MODE=$2; shift 2 ;;
    *) die "未知参数 $1" ;;
  esac
done

OUT=$OUT_DIR/device
SRC=$OUT/src
mkdir -p "$OUT"
[ -e "$PORT" ] || die "找不到串口 $PORT"
if command -v fuser >/dev/null && fuser "$PORT" >/dev/null 2>&1; then
  die "串口 $PORT 被占用（可能开着 idf.py monitor），先关掉"
fi

log "导出被测代码：$PERF_SOURCE_DESC"
perf_prepare_source "$SRC"
perf_set_config "$SRC/sdkconfig" --file "$PERF_DIR/probe/sdkconfig.probe"
perf_load_idf
log "编译测量固件"
case "$HTTP_MODE" in host | loopback) ;; *) die "--http 只能是 host 或 loopback" ;; esac
PROBE_DEFS=(-DPERF_PROBE_AUTO="$AUTO" -DPERF_PROBE_HTTP_LOOPBACK=$([ "$HTTP_MODE" = loopback ] && echo 1 || echo 0))
idf.py -C "$SRC" -B "$SRC/build" -DEXTRA_COMPONENT_DIRS="$PERF_DIR/probe/perf_probe" "${PROBE_DEFS[@]}" build >"$OUT/build.log" 2>&1 ||
  { tail -40 "$OUT/build.log"; die "编译失败，日志：$OUT/build.log"; }
APP_BIN=$(ls "$SRC"/build/*.bin | grep -v -e bootloader -e partition | head -1)

# esptool 一律在结果目录里运行，避免仓库根目录那个旧版 esptool.py 被 python 当成模块导入
cd "$OUT"
esp() { esptool.py --chip esp32 -p "$PORT" -b "$BAUD" "$@"; }
GEN=$IDF_PATH/components/partition_table/gen_esp32part.py

log "读取设备分区表"
esp read_flash 0x8000 0xC00 device-ptable.bin >/dev/null
python "$GEN" device-ptable.bin device-ptable.csv >/dev/null 2>&1
python "$GEN" "$SRC/build/partition_table/partition-table.bin" build-ptable.csv >/dev/null 2>&1
diff -q <(grep -v '^#' device-ptable.csv) <(grep -v '^#' build-ptable.csv) >/dev/null ||
  die "设备分区表和被测代码不一致（见 $OUT/*-ptable.csv），需要先完整烧录一次被测版本"
read -r APP_OFFSET APP_SIZE < <(awk -F, '!/^#/ && $2 ~ /app/ {gsub(/ /,""); print $4, $5; exit}' build-ptable.csv)
APP_SIZE_BYTES=$(python3 -c "s='$APP_SIZE'; print(int(s[:-1])*1024*(1024 if s.endswith('M') else 1) if s[-1] in 'KM' else int(s, 0))")

log "备份设备 app 分区（$APP_OFFSET，$APP_SIZE_BYTES 字节）"
CHUNK=$((0x40000))
: >app-backup.bin
for ((off = 0; off < APP_SIZE_BYTES; off += CHUNK)); do
  addr=$(printf "0x%x" $((APP_OFFSET + off)))
  len=$((APP_SIZE_BYTES - off < CHUNK ? APP_SIZE_BYTES - off : CHUNK))
  for try in 1 2 3; do
    rm -f chunk.bin
    esp read_flash "$addr" "$len" chunk.bin >/dev/null 2>&1 || true
    [ -f chunk.bin ] && [ "$(stat -c%s chunk.bin)" -eq "$len" ] && break
    [ "$try" -eq 3 ] && die "读取 $addr 失败"
  done
  cat chunk.bin >>app-backup.bin
done
rm -f chunk.bin
esptool.py --chip esp32 image_info --version 2 app-backup.bin >backup-info.txt 2>&1 ||
  die "备份的镜像校验失败，没有写入任何东西（见 $OUT/backup-info.txt）"
BACKUP_VERSION=$(sed -n 's/^App version: *//p' backup-info.txt | sed 's/ *$//')
log "备份完成：App version $BACKUP_VERSION"

log "写入测量固件"
esp --after no_reset write_flash "$APP_OFFSET" "$APP_BIN" >flash-probe.log 2>&1 ||
  { tail -5 flash-probe.log; die "写入失败；设备 app 分区可能已损坏，用 $OUT/app-backup.bin 写回 $APP_OFFSET"; }

# WSL + usbipd 下 CH340 的 DTR 偶尔会卡住，RTS 复位后芯片进下载模式（日志 boot:0x5 / waiting for download）。
# 拔插一次 USB 再重新 attach 就能恢复。启动失败时提前结束，不白等整个采集时长。
REPLUG_HINT="设备复位后进了下载模式或没有启动：拔插一次 USB，重新 usbipd attach 并放开串口权限后再运行"
CAPTURE_ARGS=(--require "PERF " --within 15)
if [ "$AUTO" = 1 ]; then
  log "采集 ${SECONDS_TO_CAPTURE} 秒，剧本自动执行（配网页请求方式：$HTTP_MODE），不需要操作设备"
  [ "$HTTP_MODE" = host ] && CAPTURE_ARGS+=(--host-http)
else
  log "采集 ${SECONDS_TO_CAPTURE} 秒，请按提示操作设备"
  CAPTURE_ARGS+=(--prompt "$PERF_DIR/scenario.txt")
fi
CAPTURE_OK=1
python "$PERF_DIR/capture_serial.py" "$PORT" "$SECONDS_TO_CAPTURE" probe.log "${CAPTURE_ARGS[@]}" || CAPTURE_OK=0

if [ "$RESTORE" = 1 ]; then
  log "写回原固件"
  esp write_flash "$APP_OFFSET" app-backup.bin >flash-restore.log 2>&1 ||
    { tail -5 flash-restore.log; die "写回失败，备份在 $OUT/app-backup.bin，偏移 $APP_OFFSET"; }
  python "$PERF_DIR/capture_serial.py" "$PORT" 10 restore.log
  RESTORED=$(sed 's/\x1b\[[0-9;]*m//g' restore.log | sed -n 's/.*App version: *//p' | head -1 | tr -d '\r' | sed 's/ *$//')
  if [ -z "$RESTORED" ] && grep -aq "waiting for download" restore.log; then
    die "原固件已写回（校验通过），但复位后进了下载模式。$REPLUG_HINT"
  fi
  [ "$RESTORED" = "$BACKUP_VERSION" ] || die "写回后启动的版本是 '$RESTORED'，和备份 '$BACKUP_VERSION' 不一致"
  log "已恢复原固件：$RESTORED"
fi
[ "$CAPTURE_OK" = 1 ] || die "测量固件没有正常启动，本次没有采到数据。$REPLUG_HINT"

python3 -I "$PERF_DIR/parse_probe_log.py" "$OUT" "$PERF_SOURCE_DESC"
log "完成：$OUT/summary.md"

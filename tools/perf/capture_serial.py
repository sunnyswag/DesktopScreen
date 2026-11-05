"""复位设备并采集串口输出：capture_serial.py <串口> <秒数> <输出文件> [--prompt 剧本文件] [--require 文本 --within 秒] [--host-http]

用 RTS 拉低 EN 复位（DTR 保持释放，IO0 为高，正常启动）。
--prompt 给出测试剧本（每行 "秒数 提示文字"），采集过程中到点在终端打印提示。
--require 要求复位后 --within 秒内出现指定文本，否则保存已收到的日志并以退出码 3 结束。
--host-http 自动剧本的阶段 3 由电脑通过局域网请求设备的配网页：从探针的 got_ip 事件取设备 IP，
            收到 auto_phase=3 事件后按 HOST_HTTP_PLAN 发请求，结果以 "HOST http ..." 行写进日志（不含 IP）。
要用带 pyserial 的 Python（ESP-IDF 的 python 环境）运行。
"""
import re
import sys
import threading
import time
import urllib.request

import serial

# 阶段 3 开始后多少秒请求哪个路径，和 probe/perf_probe/perf_scenario.c 的回环模式一致
HOST_HTTP_PLAN = [(5, "/"), (6, "/favicon.ico"), (20, "/"), (21, "/favicon.ico"), (35, "/"), (36, "/favicon.ico")]
RE_SPAN = re.compile(rb"PERF span t_ms=(\d+) dur_us=(\d+) arg=(-?\d+) name=(\w+) chk=(\d+)")


def host_http(ip, results):
    start = time.time()
    for at, path in HOST_HTTP_PLAN:
        time.sleep(max(0, start + at - time.time()))
        t0 = time.time()
        try:
            with urllib.request.urlopen(f"http://{ip}{path}", timeout=5) as resp:
                status, size = resp.status, len(resp.read())
        except Exception as e:  # 记下失败原因，不中断采集
            status, size = type(e).__name__, 0
        results.append(f"HOST http t_host={t0 - start:.1f} path={path} status={status} bytes={size} "
                       f"ms={(time.time() - t0) * 1000:.0f}\n".encode())


def main():
    port, seconds, out = sys.argv[1], float(sys.argv[2]), sys.argv[3]
    prompts = []
    if "--prompt" in sys.argv:
        for line in open(sys.argv[sys.argv.index("--prompt") + 1], encoding="utf-8"):
            line = line.strip()
            if line and not line.startswith("#"):
                at, text = line.split(maxsplit=1)
                prompts.append((float(at), text))

    require = sys.argv[sys.argv.index("--require") + 1].encode() if "--require" in sys.argv else None
    within = float(sys.argv[sys.argv.index("--within") + 1]) if "--within" in sys.argv else 15

    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, 115200, 0.2
    ser.dtr = False
    ser.rts = False
    ser.open()
    ser.rts = True
    time.sleep(0.2)
    ser.rts = False

    start = time.time()
    seen = b""
    host_http_on = "--host-http" in sys.argv
    device_ip, http_results, pending, at_line_start = None, [], b"", True
    with open(out, "wb") as f:
        while (elapsed := time.time() - start) < seconds:
            while prompts and prompts[0][0] <= elapsed:
                print(f"[{int(prompts[0][0]):>4}s] {prompts.pop(0)[1]}", flush=True)
            data = ser.read(4096)
            f.write(data)
            if data:
                at_line_start = data.endswith(b"\n")
            # 只在一行结束之后插入，免得打断串口输出的行
            while http_results and at_line_start:
                f.write(http_results.pop(0))
            if host_http_on:
                pending += data
                *lines, pending = pending.split(b"\n")
                for line in lines:
                    m = RE_SPAN.search(line)
                    if not m or (int(m.group(1)) + int(m.group(2))) % (1 << 32) != int(m.group(5)):
                        continue
                    name, arg = m.group(4), int(m.group(3)) & 0xFFFFFFFF
                    if name == b"got_ip":
                        device_ip = ".".join(str((arg >> s) & 0xFF) for s in (0, 8, 16, 24))
                    elif name == b"auto_phase" and arg == 3:
                        if device_ip:
                            print("[阶段3] 电脑通过局域网请求设备的配网页", flush=True)
                            threading.Thread(target=host_http, args=(device_ip, http_results), daemon=True).start()
                        else:
                            f.write(b"HOST http skipped reason=no_ip\n")
            if require is not None:
                seen += data
                if require in seen:
                    require = None
                elif elapsed > within:
                    download = b"waiting for download" in seen
                    print(f"设备复位后 {within:.0f} 秒内没有出现 {require.decode()!r}"
                          + ("，芯片进了下载模式（IO0 为低）" if download else ""), file=sys.stderr, flush=True)
                    ser.close()
                    sys.exit(3)
    ser.close()


if __name__ == "__main__":
    main()

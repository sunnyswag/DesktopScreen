"""size_report.sh 的汇总脚本：size_summary.py <结果目录> <被测代码描述>

读取 esp_idf_size 的 json2 输出、编译日志、nm 输出和分区表，写出 summary.md 和 metrics.json。
"""
import csv
import json
import re
import sys
from pathlib import Path

GROUPS = [
    ("WiFi/PHY 驱动与协议栈", ["libnet80211", "libpp", "libphy", "libwpa_supplicant", "libesp_wifi",
                          "libcore", "librtc", "libesp_coex", "libesp_phy"]),
    ("mbedTLS 加密/TLS", ["libmbedcrypto", "libmbedtls", "libmbedx509", "libesp-tls"]),
    ("lwIP/网络接口", ["liblwip", "libesp_netif"]),
    ("HTTP 客户端/服务器", ["libesp_http_client", "libesp_http_server", "libhttp_parser", "libtcp_transport"]),
    ("C 运行库", ["libc", "libnewlib", "libgcc", "libstdc++", "libcxx", "libm"]),
    ("存储（SPIFFS/NVS/VFS/分区）", ["libspiffs", "libnvs_flash", "libvfs", "libesp_partition", "libspi_flash"]),
    ("业务代码 main", ["libmain"]),
]
MEM_FLASH = ("Flash Code", "Flash Data")


def kb(n):
    return f"{n / 1024:.1f} KB"


def archive_mem(entry, mem):
    return entry["memory_types"].get(mem, {}).get("size", 0)


def parse_partitions(path):
    rows = []
    for row in csv.reader(open(path, encoding="utf-8")):
        if not row or row[0].strip().startswith("#"):
            continue
        name, ptype, subtype, offset, size = [c.strip() for c in row[:5]]
        rows.append((name, ptype, subtype, int(offset, 0), int(size, 0)))
    return rows


def flash_size_bytes(sdkconfig):
    m = re.search(r'^CONFIG_ESPTOOLPY_FLASHSIZE="(\d+)MB"', sdkconfig.read_text(), re.M)
    return int(m.group(1)) * 1024 * 1024 if m else 0


def main():
    out, source_desc = Path(sys.argv[1]), sys.argv[2]
    build_log = (out / "build.log").read_text(errors="replace")
    layout = {m["name"]: m for m in json.load(open(out / "size.json"))["layout"]}
    archives = json.load(open(out / "size-archives.json"))
    files = json.load(open(out / "size-files.json"))
    metrics = {}

    m = re.search(r"(\S+\.bin) binary size (0x[0-9a-f]+) bytes\. Smallest app partition is (0x[0-9a-f]+) bytes\. "
                  r"(0x[0-9a-f]+) bytes \((\d+)%\) free", build_log)
    app_bin, app_part, app_free = (int(m.group(i), 16) for i in (2, 3, 4))
    b = re.search(r"Bootloader binary size (0x[0-9a-f]+) bytes\. (0x[0-9a-f]+) bytes", build_log)
    boot_bin, boot_free = int(b.group(1), 16), int(b.group(2), 16)
    metrics.update(app_bin_bytes=app_bin, app_partition_bytes=app_part, app_free_bytes=app_free,
                   app_used_pct=round(100 * app_bin / app_part, 1), bootloader_bytes=boot_bin,
                   bootloader_free_bytes=boot_free)
    for name, key in [("Flash Code", "flash_code"), ("Flash Data", "flash_data"), ("IRAM", "iram"), ("DRAM", "dram")]:
        metrics[f"{key}_used_bytes"] = layout[name]["used"]
        if layout[name]["total"]:
            metrics[f"{key}_total_bytes"] = layout[name]["total"]
    metrics["dram_bss_bytes"] = layout["DRAM"]["parts"].get(".bss", {}).get("size", 0)
    metrics["dram_data_bytes"] = layout["DRAM"]["parts"].get(".data", {}).get("size", 0)

    # 链接器把所有可合并的字符串常量（日志、断言文件名等）归到第一个输入段，esp_idf_size 把它算在 esp_app_desc.c.obj 名下
    pool = next((v for k, v in files.items() if k.endswith("esp_app_desc.c.obj")), None)
    metrics["merged_string_pool_bytes"] = (
        pool["memory_types"]["Flash Data"]["sections"][".flash.rodata"]["size"] if pool else 0)

    lines = [f"# 静态资源评估", "", f"- 被测代码：{source_desc}", ""]

    lines += ["## 分区余量", "", "| 项目 | 大小 | 余量 |", "|---|---|---|",
              f"| app（{m.group(1)}） | {kb(app_bin)} / 分区 {kb(app_part)}（{metrics['app_used_pct']}%） | {kb(app_free)} |",
              f"| bootloader | {kb(boot_bin)} | {kb(boot_free)} |", ""]
    parts = parse_partitions(out / "partitions.csv")
    flash_total = flash_size_bytes(out / "src" / "sdkconfig")
    lines += ["| 分区 | 类型 | 偏移 | 大小 |", "|---|---|---|---|"]
    for name, ptype, subtype, offset, size in parts:
        lines.append(f"| {name} | {ptype}/{subtype} | 0x{offset:X} | {kb(size)} |")
    end = max(o + s for _, _, _, o, s in parts)
    if flash_total:
        metrics["flash_unallocated_bytes"] = flash_total - end
        lines.append(f"| （未分配） | - | 0x{end:X} | {kb(flash_total - end)} |")
    lines.append("")

    lines += ["## 内存段", "", "| 段 | 已用 | 总量 | 占比 |", "|---|---|---|---|"]
    for name in ("Flash Code", "Flash Data", "IRAM", "DRAM", "RTC FAST", "RTC SLOW"):
        if name in layout:
            seg = layout[name]
            total = seg["total"]
            pct = f"{100 * seg['used'] / total:.1f}%" if total else "-"
            detail = "，".join(f"{k} {v['size']}" for k, v in seg["parts"].items())
            lines.append(f"| {name} | {seg['used']} B（{detail}） | {total or '-'} | {pct} |")
    lines += ["", f"合并字符串池（日志/断言字符串，记在 esp_app_desc.c.obj 名下）：{kb(metrics['merged_string_pool_bytes'])}", ""]

    def short(name):
        return Path(name).name.removesuffix(".a")

    lines += ["## 按模块归类", "", "| 归类 | Flash | DRAM | IRAM |", "|---|---|---|---|"]
    grouped = set()
    for title, libs in GROUPS:
        sel = [v for k, v in archives.items() if short(k) in libs]
        grouped.update(k for k, v in archives.items() if short(k) in libs)
        flash = sum(archive_mem(v, t) for v in sel for t in MEM_FLASH)
        lines.append(f"| {title} | {kb(flash)} | {kb(sum(archive_mem(v, 'DRAM') for v in sel))} | "
                     f"{kb(sum(archive_mem(v, 'IRAM') for v in sel))} |")
        if title == "业务代码 main":
            metrics["main_flash_bytes"] = flash
            metrics["main_dram_bytes"] = sum(archive_mem(v, "DRAM") for v in sel)
    rest = [v for k, v in archives.items() if k not in grouped]
    lines.append(f"| 其他（FreeRTOS、系统、驱动、字符串池等） | {kb(sum(archive_mem(v, t) for v in rest for t in MEM_FLASH))} | "
                 f"{kb(sum(archive_mem(v, 'DRAM') for v in rest))} | {kb(sum(archive_mem(v, 'IRAM') for v in rest))} |")
    lines += ["", "### 体积前 15 的库", "", "| 库 | 合计 | Flash | DRAM | IRAM |", "|---|---|---|---|---|"]
    for k, v in sorted(archives.items(), key=lambda kv: -kv[1]["size"])[:15]:
        flash = sum(archive_mem(v, t) for t in MEM_FLASH)
        lines.append(f"| {short(k)} | {v['size']} | {flash} | {archive_mem(v, 'DRAM')} | {archive_mem(v, 'IRAM')} |")
    lines.append("")

    lines += ["## main 组件各文件", "", "| 文件 | Flash | DRAM |", "|---|---|---|"]
    main_objs = {k: v for k, v in files.items() if "libmain.a:" in k}
    for k, v in sorted(main_objs.items(), key=lambda kv: -kv[1]["size"]):
        flash = sum(archive_mem(v, t) for t in MEM_FLASH)
        lines.append(f"| {k.split(':', 1)[1]} | {flash} | {archive_mem(v, 'DRAM')} |")
    lines.append("")

    # gImage_* 符号：按地址判断落在 Flash（DROM 0x3F4xxxxx）还是 DRAM（0x3FFxxxxx）
    data_flash = data_dram = 0
    rows = []
    for line in (out / "data-symbols.txt").read_text().splitlines():
        addr, size, sym_type, name = line.split()[:4]
        addr, size = int(addr), int(size)
        in_flash = 0x3F400000 <= addr < 0x3F800000
        where = "Flash（只读）" if in_flash else ("DRAM .bss" if sym_type in "bB" else "DRAM .data（Flash 另存初值）")
        rows.append((name, size, where))
        if in_flash:
            data_flash += size
        else:
            data_dram += size
            if sym_type not in "bB":
                data_flash += size
    metrics.update(gimage_flash_bytes=data_flash, gimage_dram_bytes=data_dram)
    lines += ["## 图片数组（gImage_* 符号）", "",
              f"Flash 合计 {data_flash} B，DRAM 合计 {data_dram} B。除 gImage_last_page（paint.c 的整屏缓冲）外都定义在 main/src/data。", "",
              "| 符号 | 大小 | 位置 |", "|---|---|---|"]
    lines += [f"| {n} | {s} | {w} |" for n, s, w in sorted(rows, key=lambda r: -r[1])]
    lines.append("")

    warnings = sorted(set(re.findall(r"^(\S+?:\d+:\d+: warning: .*)$", build_log, re.M)))
    main_warn = [w for w in warnings if "/esp-idf/" not in w]
    metrics.update(warnings_total=len(warnings), warnings_main=len(main_warn))
    lines += ["## 编译告警（去重）", "", f"共 {len(warnings)} 条，其中项目代码 {len(main_warn)} 条。", ""]
    lines += [f"- `{re.sub(r'^.*?/esp-idf/', '$IDF_PATH/', re.sub(r'^.*?/main/', 'main/', w))}`" for w in warnings]
    lines.append("")

    (out / "summary.md").write_text("\n".join(lines), encoding="utf-8")
    (out / "metrics.json").write_text(json.dumps({"source": source_desc, "size": metrics}, ensure_ascii=False, indent=2),
                                      encoding="utf-8")


if __name__ == "__main__":
    main()

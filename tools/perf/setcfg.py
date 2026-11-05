"""修改 sdkconfig 里的若干项：setcfg.py <sdkconfig> KEY=VALUE ... [--file overlay]

VALUE 为 n 时写成 "# KEY is not set"。--file 读取 overlay 文件里的 KEY=VALUE 行（忽略注释）。
编译时 kconfgen 会重新解析依赖和 choice，这里只负责替换或追加。
"""
import re
import sys


def main():
    path, args = sys.argv[1], sys.argv[2:]
    sets = {}
    while args:
        arg = args.pop(0)
        if arg == "--file":
            for line in open(args.pop(0), encoding="utf-8"):
                line = line.strip()
                if line and not line.startswith("#"):
                    key, value = line.split("=", 1)
                    sets[key] = value
        else:
            key, value = arg.split("=", 1)
            sets[key] = value
    out = []
    for line in open(path, encoding="utf-8").read().splitlines():
        m = re.match(r"^(?:# )?(CONFIG_[A-Z0-9_]+)(?:=| is not set)", line)
        if m and m.group(1) in sets:
            continue
        out.append(line)
    for key, value in sets.items():
        out.append(f"# {key} is not set" if value == "n" else f"{key}={value}")
    open(path, "w", encoding="utf-8").write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()

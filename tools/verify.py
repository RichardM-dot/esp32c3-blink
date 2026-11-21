#!/usr/bin/env python3
"""verify.py — ESP32-C3 裸机镜像静态自检。

检查 C3 特有的、容易静默出错的布局约束：

  * 代码从 0x40380000 开始（0x4037C000..0x40380000 是 16 KB 指令 cache）
  * IRAM 镜像与 DRAM .bss 不通过总线别名重叠（偏移 0x700000）
  * .bss 不侵入预留栈区
  * mtvec 目标四字节对齐
  * .esp.bin 与 ELF 一致（entry/chip_id/SHA256）
"""
import argparse
import hashlib
import re
import struct
import subprocess
import sys

CODE_BASE = 0x40380000
IRAM_HIGH = 0x403E0000
DRAM_LOW = 0x3FC80000
DRAM_HIGH = 0x3FCE0000
I_D_SKEW = 0x700000
CHIP_ID_C3 = 0x0005

errors = []


def check(cond, msg):
    if not cond:
        errors.append(msg)
    print(("  ok   " if cond else "  FAIL ") + msg)


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--bin", required=True)
    ap.add_argument("--nm", required=True)
    ap.add_argument("--objdump", required=True)
    args = ap.parse_args()

    print("[1] ELF identity")
    with open(args.elf, "rb") as f:
        head = f.read(20)
    check(head[:4] == b"\x7fELF", "ELF magic")
    check(head[4] == 1, "ELF32")
    check(head[5] == 1, "little endian")
    check(struct.unpack_from("<H", head, 18)[0] == 243, "machine = RISC-V")

    print("[2] symbols")
    syms = {}
    for line in run([args.nm, "-S", args.elf]).splitlines():
        parts = line.split()
        if len(parts) >= 3 and re.fullmatch(r"[0-9a-fA-F]+", parts[0]):
            if len(parts) == 4:
                syms[parts[3]] = (int(parts[0], 16), int(parts[1], 16))
            else:
                syms[parts[2]] = (int(parts[0], 16), 0)
    for name in ("_start", "freertos_risc_v_trap_handler", "vTaskStartScheduler",
                 "xQueueGenericCreate", "pxCurrentTCBs", "_iram_end", "_bss_start",
                 "_bss_end", "_stack_top"):
        check(name in syms, f"symbol {name} present")

    print("[3] layout")
    check(syms["_start"][0] == CODE_BASE,
          f"_start at 0x{syms['_start'][0]:08X} == 0x{CODE_BASE:08X} (past the I-cache)")
    check(CODE_BASE <= syms["freertos_risc_v_trap_handler"][0] < IRAM_HIGH,
          "trap handler in IRAM")
    check(syms["freertos_risc_v_trap_handler"][0] % 4 == 0,
          "mtvec target 4-byte aligned")
    ir = syms["_iram_end"][0]
    bs = syms["_bss_start"][0]
    be = syms["_bss_end"][0]
    st = syms["_stack_top"][0]
    check(bs >= ir - I_D_SKEW,
          f".bss 0x{bs:08X} starts after IRAM alias 0x{ir - I_D_SKEW:08X}")
    check(DRAM_LOW <= bs < DRAM_HIGH, ".bss inside DRAM")
    check(be <= st - 0x4000, f".bss end 0x{be:08X} below stack 0x{st - 0x4000:08X}")
    check(syms["pxCurrentTCBs"][1] == 4, "pxCurrentTCBs is 1 pointer (single core)")

    print("[4] esp image")
    with open(args.bin, "rb") as f:
        img = f.read()
    check(img[0] == 0xE9, "image magic 0xE9")
    entry = struct.unpack_from("<I", img, 4)[0]
    check(entry == syms["_start"][0], f"image entry 0x{entry:08X} == _start")
    check(struct.unpack_from("<H", img, 12)[0] == CHIP_ID_C3, "chip id 0x0005 (C3)")
    temp = img[2]
    check(temp == 0x02, "flash mode DIO")
    segcnt = img[1]
    check(segcnt >= 1, f"image has {segcnt} segment(s)")
    off = 24
    segs = []
    for _ in range(segcnt):
        addr, size = struct.unpack_from("<II", img, off)
        segs.append((addr, size))
        off += 8 + size
    check(any(a == CODE_BASE for a, _ in segs), "a segment loads at 0x40380000")
    check(any(a <= entry < a + s for a, s in segs), "entry inside a loaded segment")
    body_len = len(img) - 32
    check(len(img) % 16 == 0, "image 16-byte aligned")
    check(hashlib.sha256(img[:body_len]).digest() == img[body_len:], "SHA256 trailer")

    print()
    if errors:
        print(f"FAILED: {len(errors)} check(s)")
        for e in errors:
            print("  - " + e)
        return 1
    print("ALL CHECKS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())

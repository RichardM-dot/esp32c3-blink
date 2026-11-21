#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""mkimage.py — 零依赖，把 ELF 打包成 ESP 官方镜像格式（中文注释）
 * 支持 ELF32 小端/大端（Xtensa 工具链的 EI_DATA 可能为 2）
 * ESP 镜像格式固定为小端
 * chip_id: c3=0x0005, s3=0x0009
"""
import argparse, hashlib, struct

CHIP_ID = {"c3": 0x0005, "s3": 0x0009}

def parse_elf(path):
    buf = open(path, "rb").read()
    assert buf[:4] == b"\x7fELF", "not ELF"
    ei_class = buf[4]
    ei_data  = buf[5]
    assert ei_class == 1, f"only ELF32 supported, got class {ei_class}"
    endian = "<" if ei_data == 1 else ">"
    print(f"[mkimage] ELF class={ei_class} data={'LE' if ei_data==1 else 'BE'}")

    e_phoff     = struct.unpack_from(endian + "I", buf, 28)[0]
    e_phentsize = struct.unpack_from(endian + "H", buf, 42)[0]
    e_phnum     = struct.unpack_from(endian + "H", buf, 44)[0]
    segs = []
    for i in range(e_phnum):
        off = e_phoff + i * e_phentsize
        p_type   = struct.unpack_from(endian + "I", buf, off)[0]
        if p_type != 1:  # PT_LOAD
            continue
        p_offset = struct.unpack_from(endian + "I", buf, off + 4)[0]
        p_paddr  = struct.unpack_from(endian + "I", buf, off + 12)[0]
        p_filesz = struct.unpack_from(endian + "I", buf, off + 16)[0]
        if p_filesz == 0:
            continue
        segs.append((p_paddr, buf[p_offset:p_offset + p_filesz]))
    return segs

def build_image(segs, entry, chip_id):
    """ESP 镜像格式固定小端"""
    hdr = bytearray(24)
    hdr[0] = 0xE9
    hdr[1] = len(segs) & 0xFF
    # SPI 模式/速度/容量: DIO, 40 MHz, 4 MB。ROM 直接启动时靠这三字节
    # 初始化 flash 才能继续读镜像；全 0 表示 QIO + 1 MB，部分 flash 读不了。
    hdr[2] = 0x02
    hdr[3] = 0x20
    struct.pack_into("<I", hdr, 4, entry)
    struct.pack_into("<H", hdr, 12, chip_id)
    hdr[8] = 0xEE   # wp_pin: 0xEE = 无写保护引脚（与 esptool 一致）
    hdr[23] = 1  # hash_appended
    body = bytearray(hdr)
    for addr, data in segs:
        body += struct.pack("<II", addr, len(data))
        body += data
    while len(body) % 16:
        body += b"\x00"
    body += hashlib.sha256(body).digest()
    return bytes(body)

def verify(path, entry, chip_id, segcnt):
    b = open(path, "rb").read()
    assert b[0] == 0xE9, "magic != 0xE9"
    assert b[1] == segcnt, "segcnt mismatch"
    assert struct.unpack_from("<I", b, 4)[0] == entry, "entry mismatch"
    assert struct.unpack_from("<H", b, 12)[0] == chip_id, "chip_id mismatch"
    assert b[23] == 1, "hash flag"
    assert hashlib.sha256(b[:-32]).digest() == b[-32:], "SHA256 mismatch"
    print(f"[verify] OK magic=0xE9 segs={b[1]} entry=0x{entry:08X} chip_id=0x{chip_id:04X} sha256=match")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--entry", required=True)
    ap.add_argument("--chip", required=True, choices=list(CHIP_ID))
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    entry = int(a.entry, 0)
    segs = parse_elf(a.elf)
    img = build_image(segs, entry, CHIP_ID[a.chip])
    open(a.out, "wb").write(img)
    print(f"[mkimage] {a.out}  size={len(img)}  segs={len(segs)}")
    verify(a.out, entry, CHIP_ID[a.chip], len(segs))

if __name__ == "__main__":
    main()

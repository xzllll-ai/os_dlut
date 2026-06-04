#!/usr/bin/env python3
import math
import os
import struct
import sys


SECTOR_SIZE = 512
TOTAL_SECTORS = 128 * 1024
RESERVED_SECTORS = 32
FAT_COPIES = 2
SECTORS_PER_FAT = 1024
SECTORS_PER_CLUSTER = 8
ROOT_CLUSTER = 2
EOC = 0x0FFFFFFF


def put_le16(buf, off, value):
    struct.pack_into("<H", buf, off, value)


def put_le32(buf, off, value):
    struct.pack_into("<I", buf, off, value)


def make_bootsector():
    bs = bytearray(SECTOR_SIZE)
    bs[0:3] = b"\xEB\x58\x90"
    bs[3:11] = b"MSDOS5.0"
    put_le16(bs, 11, SECTOR_SIZE)
    bs[13] = SECTORS_PER_CLUSTER
    put_le16(bs, 14, RESERVED_SECTORS)
    bs[16] = FAT_COPIES
    put_le16(bs, 17, 0)
    put_le16(bs, 19, 0)
    bs[21] = 0xF8
    put_le16(bs, 22, 0)
    put_le16(bs, 24, 63)
    put_le16(bs, 26, 255)
    put_le32(bs, 28, 0)
    put_le32(bs, 32, TOTAL_SECTORS)
    put_le32(bs, 36, SECTORS_PER_FAT)
    put_le16(bs, 40, 0)
    put_le16(bs, 42, 0)
    put_le32(bs, 44, ROOT_CLUSTER)
    put_le16(bs, 48, 1)
    put_le16(bs, 50, 6)
    bs[64] = 0x80
    bs[66] = 0x29
    put_le32(bs, 67, 0x20260603)
    bs[71:82] = b"SYSTEM     "
    bs[82:90] = b"FAT32   "
    bs[510:512] = b"\x55\xAA"
    return bs


def make_dir_entry(name, cluster, size):
    entry = bytearray(32)
    base, ext = split_83(name)
    entry[0:8] = base
    entry[8:11] = ext
    entry[11] = 0x20
    entry[12] = 0x08
    put_le16(entry, 20, (cluster >> 16) & 0xFFFF)
    put_le16(entry, 26, cluster & 0xFFFF)
    put_le32(entry, 28, size)
    return entry


def split_83(name):
    upper = name.upper()
    if "." in upper:
        base, ext = upper.split(".", 1)
    else:
        base, ext = upper, ""
    if not base or len(base) > 8 or len(ext) > 3:
        raise ValueError(f"{name!r} is not an 8.3 FAT name")
    return base.encode("ascii").ljust(8, b" "), ext.encode("ascii").ljust(3, b" ")


def parse_file_arg(arg):
    if ":" not in arg:
        raise ValueError(f"expected SRC:DEST, got {arg!r}")
    src, dst = arg.split(":", 1)
    return src, dst


def main(argv):
    if len(argv) < 3:
        print("usage: build-fat32-img.py OUTPUT SRC:DEST...", file=sys.stderr)
        return 2

    output = argv[1]
    files = [parse_file_arg(arg) for arg in argv[2:]]
    data_start = RESERVED_SECTORS + FAT_COPIES * SECTORS_PER_FAT
    cluster_count = TOTAL_SECTORS - data_start
    fat_entries = SECTORS_PER_FAT * SECTOR_SIZE // 4
    if cluster_count + 2 > fat_entries:
        raise RuntimeError("FAT is too small for configured image")

    os.makedirs(os.path.dirname(output) or ".", exist_ok=True)
    image = bytearray(TOTAL_SECTORS * SECTOR_SIZE)
    image[0:SECTOR_SIZE] = make_bootsector()

    fat = [0] * fat_entries
    fat[0] = 0x0FFFFFF8
    fat[1] = EOC
    fat[ROOT_CLUSTER] = EOC

    root = bytearray(SECTOR_SIZE * SECTORS_PER_CLUSTER)
    next_cluster = ROOT_CLUSTER + 1
    for idx, (src, dst) in enumerate(files):
        data = open(src, "rb").read()
        clusters = max(1, math.ceil(len(data) / (SECTOR_SIZE * SECTORS_PER_CLUSTER)))
        first = next_cluster
        for offset in range(clusters):
            cluster = next_cluster + offset
            fat[cluster] = cluster + 1 if offset + 1 < clusters else EOC
            chunk = data[
                offset * SECTOR_SIZE * SECTORS_PER_CLUSTER :
                (offset + 1) * SECTOR_SIZE * SECTORS_PER_CLUSTER
            ]
            sector = data_start + (cluster - 2) * SECTORS_PER_CLUSTER
            start = sector * SECTOR_SIZE
            image[start : start + len(chunk)] = chunk
        next_cluster += clusters
        root[idx * 32 : (idx + 1) * 32] = make_dir_entry(dst, first, len(data))

    root_sector = data_start + (ROOT_CLUSTER - 2) * SECTORS_PER_CLUSTER
    image[root_sector * SECTOR_SIZE : (root_sector + SECTORS_PER_CLUSTER) * SECTOR_SIZE] = root

    fat_bytes = bytearray(SECTORS_PER_FAT * SECTOR_SIZE)
    for idx, value in enumerate(fat):
        struct.pack_into("<I", fat_bytes, idx * 4, value)
    for copy in range(FAT_COPIES):
        sector = RESERVED_SECTORS + copy * SECTORS_PER_FAT
        start = sector * SECTOR_SIZE
        image[start : start + len(fat_bytes)] = fat_bytes

    with open(output, "wb") as f:
        f.write(image)

    print(f"created {output} with {len(files)} file(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

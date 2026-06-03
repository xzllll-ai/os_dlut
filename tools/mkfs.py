#!/usr/bin/env python3
"""Create a tiny RAMFS image for the teaching OS.

The running kernel builds its default RAMFS in memory. This host tool is kept
for the course requirement and emits a simple inspectable image format:
magic, file count, then repeated fixed headers plus file bytes.
"""

import os
import struct
import sys

MAGIC = b"DLUTFS1\0"
MAX_FILE = 64 * 1024


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: mkfs.py image input-file...", file=sys.stderr)
        return 2
    out = sys.argv[1]
    files = sys.argv[2:]
    with open(out, "wb") as image:
        image.write(MAGIC)
        image.write(struct.pack("<I", len(files)))
        for path in files:
            data = open(path, "rb").read()
            if len(data) > MAX_FILE:
                raise SystemExit(f"{path}: file exceeds 64KB")
            name = ("/" + os.path.basename(path)).encode()
            if len(name) > 63:
                raise SystemExit(f"{path}: name too long")
            image.write(struct.pack("<64sI", name, len(data)))
            image.write(data)
    print(f"created {out} with {len(files)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

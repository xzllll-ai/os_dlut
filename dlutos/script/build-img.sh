#!/bin/sh

OS=`uname -s`

if sudo --version > /dev/null 2>&1; then
    SUDO=sudo
fi

if [ "$OUTPUT" = "" ]; then
    OUTPUT="build/fs-$ARCH.img"
fi

if [ "$ARCH" = "" ]; then
    echo "ARCH is not set, exiting..." >&2
    exit 1
fi

if [ "$ARCH" = "riscv64" ] && [ "$DLUTOS_USE_SUDO_IMG" != "1" ]; then
    riscv64-linux-gnu-gcc -static -O2 -o build/tinyshell.riscv64 \
        ./user-programs/tinyshell.c
    python3 script/build-fat32-img.py "$OUTPUT" \
        ./user-programs/busybox.static:busybox \
        ./user-programs/init_script_riscv64.sh:initsh \
        build/tinyshell.riscv64:tshell
    exit
fi

dd if=/dev/zero of="$OUTPUT" bs=`expr 1024 \* 1024` count=1020
mkfs.fat -n SYSTEM "$OUTPUT"

if [ "$OS" = "Darwin" ]; then
    SUDO=''
    hdiutil detach build/mnt > /dev/null 2>&1 || true
    hdiutil attach "$OUTPUT" -mountpoint build/mnt
else
    mkdir -p build/mnt
    $SUDO losetup -P /dev/loop2 "$OUTPUT"
    $SUDO mount /dev/loop2 build/mnt
fi

if [ "$ARCH" = "riscv64" ]; then
    riscv64-linux-gnu-gcc -static -O2 -o build/tinyshell.riscv64 \
        ./user-programs/tinyshell.c
    $SUDO cp ./user-programs/busybox.static build/mnt/busybox
    $SUDO cp ./user-programs/init_script_riscv64.sh build/mnt/initsh
    $SUDO cp build/tinyshell.riscv64 build/mnt/tshell
fi

# Add your custom files here


# End of custom files

if [ "$OS" = "Darwin" ]; then
    hdiutil detach build/mnt
else
    $SUDO losetup -d /dev/loop2
    $SUDO umount build/mnt
fi

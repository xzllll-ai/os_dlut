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

prepare_riscv_libs() {
    mkdir -p build/riscv-libs
    if [ ! -f build/riscv-libs/libssl.so.3 ] || [ ! -f build/riscv-libs/libcrypto.so.3 ]; then
        debugfs -R 'dump -p /usr/lib/libssl.so.3 build/riscv-libs/libssl.so.3' \
            ../alpine-linux-riscv64-ext4fs.img >/dev/null 2>&1 || true
        debugfs -R 'dump -p /usr/lib/libcrypto.so.3 build/riscv-libs/libcrypto.so.3' \
            ../alpine-linux-riscv64-ext4fs.img >/dev/null 2>&1 || true
    fi
    ln -sf libssl.so.3 build/riscv-libs/libssl.so
    ln -sf libcrypto.so.3 build/riscv-libs/libcrypto.so
}

prepare_riscv_curl() {
    mkdir -p build/apk-curl/extract
    if [ ! -f build/apk-curl/extract/usr/bin/curl ]; then
        wget -q -O build/apk-curl/curl.apk \
            https://dl-cdn.alpinelinux.org/alpine/v3.22/main/riscv64/curl-8.14.1-r2.apk
        (cd build/apk-curl/extract && tar -xzf ../curl.apk 2>/dev/null || tar -xf ../curl.apk)
    fi
}

if [ "$ARCH" = "riscv64" ] && [ "$DLUTOS_USE_SUDO_IMG" != "1" ]; then
    prepare_riscv_libs
    prepare_riscv_curl
    riscv64-linux-gnu-gcc -static -O2 -o build/tinyshell.riscv64 \
        ./user-programs/tinyshell.c
    riscv64-linux-gnu-gcc -static -O2 -o build/minivim.riscv64 \
        ./user-programs/minivim.c
    riscv64-linux-gnu-gcc -static -O2 -o build/lswrap.riscv64 \
        ./user-programs/lswrap.c
    set -- \
        ./user-programs/busybox.static:busybox \
        ./user-programs/init_script_riscv64.sh:initsh \
        ./user-programs/all_test.sh:all_test.sh \
        ./user-programs/git:git \
        ./user-programs/git_test.sh:git_test.sh \
        build/minivim.riscv64:vim \
        ./user-programs/vim_test.sh:vim_test.sh \
        ./user-programs/gcc:gcc \
        ./user-programs/gcc_test.sh:gcc_test.sh \
        ./user-programs/aout.tpl:aout.tpl \
        ./user-programs/rustc:rustc \
        ./user-programs/rst_test.sh:rst_test.sh \
        build/lswrap.riscv64:lswrap \
        build/apk-curl/extract/usr/bin/curl:curl \
        build/tinyshell.riscv64:tshell
    if [ -f ./user-programs/git_env.local ]; then
        set -- "$@" ./user-programs/git_env.local:git_env
    fi
    python3 script/build-fat32-img.py "$OUTPUT" "$@"
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
    prepare_riscv_libs
    prepare_riscv_curl
    riscv64-linux-gnu-gcc -static -O2 -o build/tinyshell.riscv64 \
        ./user-programs/tinyshell.c
    riscv64-linux-gnu-gcc -static -O2 -o build/minivim.riscv64 \
        ./user-programs/minivim.c
    riscv64-linux-gnu-gcc -static -O2 -o build/lswrap.riscv64 \
        ./user-programs/lswrap.c
    $SUDO cp ./user-programs/busybox.static build/mnt/busybox
    $SUDO cp ./user-programs/init_script_riscv64.sh build/mnt/initsh
    $SUDO cp ./user-programs/all_test.sh build/mnt/all_test.sh
    $SUDO cp ./user-programs/git build/mnt/git
    $SUDO cp ./user-programs/git_test.sh build/mnt/git_test.sh
    $SUDO cp build/minivim.riscv64 build/mnt/vim
    $SUDO cp ./user-programs/vim_test.sh build/mnt/vim_test.sh
    $SUDO cp ./user-programs/gcc build/mnt/gcc
    $SUDO cp ./user-programs/gcc_test.sh build/mnt/gcc_test.sh
    $SUDO cp ./user-programs/aout.tpl build/mnt/aout.tpl
    $SUDO cp ./user-programs/rustc build/mnt/rustc
    $SUDO cp ./user-programs/rst_test.sh build/mnt/rst_test.sh
    $SUDO cp build/lswrap.riscv64 build/mnt/lswrap
    $SUDO cp build/apk-curl/extract/usr/bin/curl build/mnt/curl
    if [ -f ./user-programs/git_env.local ]; then
        $SUDO cp ./user-programs/git_env.local build/mnt/git_env
    fi
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

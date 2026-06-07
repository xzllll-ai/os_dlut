# os_dlut

This repository keeps the RISC-V DLUTos system as the active implementation.

## Build and Run

Requirements:

- `rustup`
- `cargo`
- `rustc`
- `mkfs.fat`
- `qemu-system-riscv64`
- `cargo objcopy`

Commands:

```sh
make dlutos-check
make dlutos-run
```

`make dlutos-run` builds DLUTos for `riscv64`, creates the FAT boot image, attaches
`alpine-linux-riscv64-ext4fs.img` as the secondary disk, and starts QEMU.

Inside the DLUTos shell, a basic validation flow is:

```sh
uname -m
cd /mnt1
rm -rf my-folder
git clone --depth=1 https://gitee.com/oscomp/xv6-riscv.git my-folder
```

The current active code is under `dlutos/`. The other legacy teaching-kernel
directories were removed because they were only used by the old root Makefile
targets and were not part of the DLUTos run path.

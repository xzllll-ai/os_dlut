# DLUT RISC-V Teaching OS

This is a small RISC-V 64 teaching kernel for QEMU `virt`.

## Build and Run

Requirements:

- `riscv64-linux-gnu-gcc`
- `qemu-system-riscv64`
- `make`

Commands:

```sh
make
make run
```

The kernel is loaded at `0x80000000` with `-bios none`. It starts in machine
mode, configures PMP/delegation/timer, then switches to supervisor mode and
enters `kernel_main`.

## Official Alpine Image

If the course-provided `alpine-linux-riscv64-ext4fs.img` is present, inspect it
with:

```sh
make img-info
```

It is a raw ext4 RISC-V Alpine root filesystem. It contains Linux user-space
tools such as `git`, `vim`, `gcc`, and `rustc`, but it is not a standalone
bootable disk for this teaching kernel. The current kernel uses RAMFS, so it
cannot read ext4 or execute Linux binaries from that image yet.

The image can be attached to QEMU as a virtio block device for future
virtio-blk/ext4 work:

```sh
make run-with-official-img
```

This only attaches the disk. Reading files from it still requires implementing a
block driver and an ext4 reader, or booting a separate Linux kernel with this
rootfs.

## Implemented Features

- Boot: M-mode entry, stack setup, trap vector setup, M-to-S transition.
- Interrupts/exceptions: M timer interrupt at 100 Hz with a pure-assembly
  scratch save path, S trap vector, syscall dispatch through `ecall`, user
  exception isolation and page-fault handling path. PLIC + UART IRQ code is
  included; polling is the default console path for stable QEMU `-bios none`
  runs.
- Memory: FDT memory-region detection, 4 MB physical page-frame allocator,
  4 KB pages, kernel heap `kmalloc/kfree`, Sv39 identity map for kernel/MMIO,
  demand-page handler.
- Processes: PCB table, parent pid, ready/running/blocked/zombie states,
  FCFS and RR scheduler modes, `fork`, `wait`, `kill`, semaphore and mutex APIs.
- Filesystem: RAMFS with 128 nodes, 64 KB max file size, nested directories,
  open/close/read/write/seek, path parsing, create/delete.
- User programs: ELF64/RISC-V loader plus five built-in U-mode user programs:
  `hello`, `writer`, `counter`, `fault`, `memtest`. Built-ins enter U-mode
  with `sret` and use `ecall` to return to the kernel. `/bin/hello_ext.elf` is
  built as a real external ELF and embedded into RAMFS; the loader maps PT_LOAD
  pages and a user stack into a per-process Sv39 page table. The paged external
  ELF execution path is still marked experimental, but the M timer path no
  longer depends on the interrupted stack.
- Shell: `ls`, `cat`, `echo`, `ps`, `kill`, `exec`, `progs`, `sched`, `mem`,
  `bench`, `touch`, `write`, `rm`, `ticks`, `fork`, `wait`, simple redirection
  (`echo TEXT > file`, `cat < file`), and a simple `echo TEXT | cat` pipeline.
- User libc: `include/ulib.h` provides simple syscall wrappers for user code.
- `tools/mkfs.py`: host-side filesystem image generator for the course
  requirement.

## Demo Commands

```text
ls /
cat /etc/motd
progs
exec hello abc
exec counter 3
exec writer
cat /home/user/generated.txt
exec fault
sched fcfs
ps
echo hello | cat
echo saved > tmp.txt
cat < tmp.txt
bench
```

## Notes

QEMU RISC-V `virt` uses a serial console in `-nographic` mode, so terminal
keyboard input is handled through the NS16550 UART instead of an x86 PS/2
keyboard controller.

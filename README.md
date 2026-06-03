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

## Implemented Features

- Boot: M-mode entry, stack setup, trap vector setup, M-to-S transition.
- Interrupts/exceptions: M timer interrupt at 100 Hz, S trap vector, syscall
  dispatch through `ecall`, page-fault handling path.
- Memory: 4 MB physical page-frame allocator, 4 KB pages, kernel heap
  `kmalloc/kfree`, Sv39 identity map for kernel/MMIO, demand-page handler.
- Processes: PCB table, parent pid, ready/running/blocked/zombie states,
  FCFS and RR scheduler modes, `wait`, `kill`, semaphore and mutex APIs.
- Filesystem: RAMFS with 128 nodes, 64 KB max file size, nested directories,
  open/close/read/write/seek, path parsing, create/delete.
- User programs: ELF64/RISC-V parser plus five built-in user programs:
  `hello`, `writer`, `counter`, `fault`, `memtest`.
- Shell: `ls`, `cat`, `echo`, `ps`, `kill`, `exec`, `progs`, `sched`, `mem`,
  `touch`, `write`, `rm`, `ticks`, and a simple `echo TEXT | cat` pipeline.
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
sched fcfs
ps
echo hello | cat
```

## Notes

QEMU RISC-V `virt` uses a serial console in `-nographic` mode, so terminal
keyboard input is handled through the NS16550 UART instead of an x86 PS/2
keyboard controller.

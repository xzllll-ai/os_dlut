CROSS ?= riscv64-linux-gnu-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

BUILD := build
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin

CFLAGS := -std=gnu11 -Wall -Wextra -Werror -O2 -g
CFLAGS += -ffreestanding -fno-common -fno-stack-protector -fno-pic -no-pie
CFLAGS += -mcmodel=medany -march=rv64gc -mabi=lp64
CPPFLAGS := -Iinclude
LDFLAGS := -T linker.ld -nostdlib -static -z max-page-size=4096

SRCS := \
	boot/entry.S \
	kernel/console.c \
	kernel/elf.c \
	kernel/fs.c \
	kernel/kalloc.c \
	kernel/main.c \
	kernel/mm.c \
	kernel/plic.c \
	kernel/proc.c \
	kernel/sbi.c \
	kernel/shell.c \
	kernel/syscall.c \
	kernel/trap.c \
	kernel/uart.c \
	kernel/user.c \
	lib/printf.c \
	lib/string.c

OBJS := $(patsubst %.S,$(BUILD)/%.o,$(patsubst %.c,$(BUILD)/%.o,$(SRCS)))

.PHONY: all run debug clean disasm mkfs

all: $(KERNEL_ELF) $(KERNEL_BIN)

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

run: $(KERNEL_ELF)
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios none -kernel $(KERNEL_ELF)

debug: $(KERNEL_ELF)
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios none -kernel $(KERNEL_ELF) -S -s

disasm: $(KERNEL_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD)/kernel.asm

mkfs:
	python3 tools/mkfs.py rootfs.img README.md

clean:
	rm -rf $(BUILD) rootfs.img

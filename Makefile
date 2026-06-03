CROSS ?= riscv64-linux-gnu-
CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump

BUILD := build
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin
EXT_ELF := $(BUILD)/user/hello_ext.elf
EXT_BLOB := $(BUILD)/user/hello_ext_blob.o
OFFICIAL_IMG ?= alpine-linux-riscv64-ext4fs.img

CFLAGS := -std=gnu11 -Wall -Wextra -Werror -O2 -g
CFLAGS += -ffreestanding -fno-common -fno-stack-protector -fno-pic -no-pie
CFLAGS += -mcmodel=medany -march=rv64gc -mabi=lp64
CPPFLAGS := -Iinclude
LDFLAGS := -T linker.ld -nostdlib -static -z max-page-size=4096

SRCS := \
	boot/entry.S \
	kernel/console.c \
	kernel/embedded.c \
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

OBJS := $(patsubst %.S,$(BUILD)/%.o,$(patsubst %.c,$(BUILD)/%.o,$(SRCS))) $(EXT_BLOB)

.PHONY: all run run-with-official-img debug clean disasm mkfs img-info

all: $(KERNEL_ELF) $(KERNEL_BIN)

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(EXT_ELF): user/hello_ext.S user/user.ld
	@mkdir -p $(dir $@)
	$(CC) -nostdlib -static -ffreestanding -fno-pic -no-pie -mcmodel=medany -march=rv64gc -mabi=lp64 -T user/user.ld $< -o $@

$(EXT_BLOB): $(EXT_ELF)
	@mkdir -p $(dir $@)
	$(LD) -r -b binary $< -o $@

$(KERNEL_ELF): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

run: $(KERNEL_ELF)
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios none -kernel $(KERNEL_ELF)

run-with-official-img: $(KERNEL_ELF)
	@test -f $(OFFICIAL_IMG) || (echo "missing $(OFFICIAL_IMG)"; exit 1)
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios none -kernel $(KERNEL_ELF) -drive file=$(OFFICIAL_IMG),format=raw,if=none,id=official0 -device virtio-blk-device,drive=official0

debug: $(KERNEL_ELF)
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios none -kernel $(KERNEL_ELF) -S -s

disasm: $(KERNEL_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD)/kernel.asm

mkfs:
	python3 tools/mkfs.py rootfs.img README.md

img-info:
	@test -f $(OFFICIAL_IMG) || (echo "missing $(OFFICIAL_IMG)"; exit 1)
	file $(OFFICIAL_IMG)
	qemu-img info $(OFFICIAL_IMG)
	@command -v debugfs >/dev/null && debugfs -R 'ls -l /' $(OFFICIAL_IMG) || true

clean:
	rm -rf $(BUILD) rootfs.img

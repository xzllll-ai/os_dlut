.PHONY: all
all:
	OUT="Makefile.real" ./configure

	make -f Makefile.real \
		build/riscv64gc-unknown-none-elf/release/dlutos_kernel \
		ARCH=riscv64 MODE=release
	
	make -f Makefile.real build/boot-riscv64.img
	
	cp build/riscv64gc-unknown-none-elf/release/dlutos_kernel \
		kernel-rv
	
	mv build/boot-riscv64.img disk.img

.PHONY: run-rv
run-rv:
	qemu-system-riscv64 -machine virt -kernel kernel-rv -m 1G -nographic -bios default \
		-drive file=sdcard-rv.img,if=none,format=raw,id=x0 \
		-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -no-reboot \
		-device virtio-net-device,netdev=net -netdev user,id=net -rtc base=utc \
		-drive file=disk.img,if=none,format=raw,id=x1 \
		-device virtio-blk-device,drive=x1,bus=virtio-mmio-bus.1

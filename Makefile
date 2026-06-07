OFFICIAL_IMG ?= alpine-linux-riscv64-ext4fs.img
DLUTOS_DIR ?= dlutos

.PHONY: all dlutos-check dlutos-configure dlutos-build dlutos-run img-info clean

all: dlutos-build

dlutos-check:
	@command -v rustup >/dev/null || (echo "missing rustup"; exit 1)
	@command -v cargo >/dev/null || (echo "missing cargo"; exit 1)
	@command -v rustc >/dev/null || (echo "missing rustc"; exit 1)
	@command -v mkfs.fat >/dev/null || (echo "missing mkfs.fat from dosfstools"; exit 1)
	@command -v qemu-system-riscv64 >/dev/null || (echo "missing qemu-system-riscv64"; exit 1)
	@cargo objcopy --version >/dev/null 2>&1 || (echo "missing cargo-binutils or llvm-tools-preview"; exit 1)
	@test -d $(DLUTOS_DIR) || (echo "missing $(DLUTOS_DIR)"; exit 1)
	@test -f $(OFFICIAL_IMG) || (echo "missing $(OFFICIAL_IMG)"; exit 1)

dlutos-configure:
	cd $(DLUTOS_DIR) && OUT=Makefile.real ARCH=riscv64 ./configure

dlutos-build: dlutos-check dlutos-configure
	$(MAKE) -C $(DLUTOS_DIR) -f Makefile.real build ARCH=riscv64 MODE=release

dlutos-run: dlutos-build
	$(MAKE) -C $(DLUTOS_DIR) -f Makefile.real test-run ARCH=riscv64 MODE=release IMG=$(abspath $(OFFICIAL_IMG)) QEMU=qemu-system-riscv64 QEMU_ACCEL="-accel tcg"

img-info:
	@test -f $(OFFICIAL_IMG) || (echo "missing $(OFFICIAL_IMG)"; exit 1)
	file $(OFFICIAL_IMG)
	qemu-img info $(OFFICIAL_IMG)
	@command -v debugfs >/dev/null && debugfs -R 'ls -l /' $(OFFICIAL_IMG) || true

clean:
	rm -rf $(DLUTOS_DIR)/build $(DLUTOS_DIR)/Makefile.real

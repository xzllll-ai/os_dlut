#[cfg(not(target_arch = "riscv64"))]
compile_error!("Goldfish RTC driver is only supported on RISC-V architecture");

pub fn probe() {
    // Some QEMU RISC-V virt configurations do not expose the legacy Goldfish
    // RTC at the old hardcoded MMIO address. Leaving RTC unregistered is safe:
    // Instant::now() falls back to ticks since boot.
}

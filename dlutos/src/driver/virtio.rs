mod hal;
mod virtio_blk;
mod virtio_net;

pub use virtio_net::VIRTIO_NET_NAME;

#[cfg(not(target_arch = "riscv64"))]
compile_error!("VirtIO drivers are only supported on RISC-V architecture");

#[cfg(target_arch = "riscv64")]
mod riscv64;

pub fn init_virtio_devices() {
    #[cfg(target_arch = "riscv64")]
    riscv64::init();
}

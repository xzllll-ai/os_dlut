#![allow(warnings)]
cfg_if::cfg_if! {
    if #[cfg(target_arch = "riscv64")] {
        pub mod riscv64;
        pub use riscv64::*;
    } else {
        compile_error!("Unsupported architecture");
    }
}

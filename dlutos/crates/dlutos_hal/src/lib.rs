#![no_std]
#![feature(allocator_api)]
#![feature(doc_notable_trait)]
#![feature(impl_trait_in_assoc_type)]

pub(crate) mod arch;

pub mod bootstrap;
pub mod context;
pub mod mm;
pub mod platform;
pub mod trap;

pub mod fence {
    pub use crate::arch::fence::{memory_barrier, read_memory_barrier, write_memory_barrier};
}

pub mod fpu {
    pub use crate::arch::fpu::FpuState;
}

pub mod processor {
    pub use crate::arch::cpu::{halt, UserTLS, CPU, CPU_COUNT};
}

/// Re-export the arch module for use in other crates
///
/// Use this to access RISC-V-specific functionality from other crates.
pub mod arch_exported {
    pub use crate::arch::*;
}

pub use dlutos_hal_macros::{ap_main, default_trap_handler, main};
pub use dlutos_hal_traits as traits;

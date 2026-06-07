use super::SerialRegister;
use core::ptr::NonNull;
use dlutos_hal::mm::ArchPhysAccess;
use dlutos_mm::address::{PAddr, PhysAccess};

pub struct SerialIO {
    base_addr: NonNull<u8>,
}

unsafe impl Send for SerialIO {}

unsafe impl Sync for SerialIO {}

impl SerialRegister for NonNull<u8> {
    fn read(&self) -> u8 {
        // SAFETY: `self` is a valid pointer to the serial port register.
        unsafe { self.as_ptr().read_volatile() }
    }

    fn write(&self, data: u8) {
        // SAFETY: `self` is a valid pointer to the serial port register.
        unsafe { self.as_ptr().write_volatile(data) };
    }
}

impl SerialIO {
    /// Creates a new `SerialIO` instance with the given physical address.
    ///
    /// # Safety
    /// This function is unsafe because it assumes that the provided `base_addr` is a valid
    /// physical address for the serial port. The caller must ensure that this address is correct
    /// and that the memory at this address is accessible.
    pub unsafe fn new(base_addr: PAddr) -> Self {
        Self {
            base_addr: unsafe {
                // SAFETY: `base_addr` is a valid physical address for the serial port.
                ArchPhysAccess::as_ptr(base_addr)
            },
        }
    }

    pub fn tx_rx(&self) -> impl SerialRegister {
        self.base_addr
    }

    pub fn int_ena(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(1) }
    }

    pub fn int_ident(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(2) }
    }

    pub fn line_control(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(3) }
    }

    pub fn modem_control(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(4) }
    }

    pub fn line_status(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(5) }
    }

    pub fn modem_status(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(6) }
    }

    pub fn scratch(&self) -> impl SerialRegister {
        unsafe { self.base_addr.add(7) }
    }
}

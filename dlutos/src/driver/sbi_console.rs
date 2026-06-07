use crate::kernel::{Terminal, TerminalDevice};
use alloc::sync::Arc;
use dlutos_log::ConsoleWrite;

struct SbiConsole;

impl ConsoleWrite for SbiConsole {
    fn write(&self, s: &str) {
        dlutos_hal::bootstrap::early_console_write(s);
    }
}

impl TerminalDevice for SbiConsole {
    fn write(&self, data: &[u8]) {
        for &ch in data {
            dlutos_hal::bootstrap::early_console_putchar(ch);
        }
    }

    fn write_direct(&self, data: &[u8]) {
        for &ch in data {
            dlutos_hal::bootstrap::early_console_putchar(ch);
        }
    }
}

pub fn init_console() {
    dlutos_log::set_console(Arc::new(SbiConsole));

    let console = Arc::new(SbiConsole);
    let terminal = Terminal::new(console.clone());
    crate::kernel::console::set_console(terminal).expect("Failed to set console");
}

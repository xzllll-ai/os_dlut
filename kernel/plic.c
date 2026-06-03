#include "plic.h"
#include "riscv.h"
#include "uart.h"

#define PLIC_PRIORITY(irq) (PLIC + (irq) * 4)
#define PLIC_SENABLE      (PLIC + 0x2080)
#define PLIC_STHRESHOLD   (PLIC + 0x201000)
#define PLIC_SCLAIM       (PLIC + 0x201004)

static inline u32 mmio_read32(u64 addr) {
    return *(volatile u32 *)addr;
}

static inline void mmio_write32(u64 addr, u32 value) {
    *(volatile u32 *)addr = value;
}

void plic_init(void) {
    mmio_write32(PLIC_PRIORITY(UART0_IRQ), 1);
    mmio_write32(PLIC_SENABLE, mmio_read32(PLIC_SENABLE) | (1U << UART0_IRQ));
    mmio_write32(PLIC_STHRESHOLD, 0);
}

void plic_handle(void) {
    u32 irq = mmio_read32(PLIC_SCLAIM);
    if (irq == UART0_IRQ) {
        uart_intr();
    }
    if (irq) {
        mmio_write32(PLIC_SCLAIM, irq);
    }
}

#include "riscv.h"
#include "uart.h"

#define RHR 0
#define THR 0
#define IER 1
#define FCR 2
#define LCR 3
#define LSR 5

#define LSR_RX_READY 0x01
#define LSR_TX_IDLE  0x20

void uart_init(void) {
    mmio_write8(UART0 + IER, 0x00);
    mmio_write8(UART0 + LCR, 0x80);
    mmio_write8(UART0 + 0, 0x03);
    mmio_write8(UART0 + 1, 0x00);
    mmio_write8(UART0 + LCR, 0x03);
    mmio_write8(UART0 + FCR, 0x07);
    mmio_write8(UART0 + IER, 0x01);
}

void uart_putc(int ch) {
    if (ch == '\n') {
        uart_putc('\r');
    }
    while ((mmio_read8(UART0 + LSR) & LSR_TX_IDLE) == 0) {
    }
    mmio_write8(UART0 + THR, (u8)ch);
}

int uart_getc_nonblock(void) {
    if ((mmio_read8(UART0 + LSR) & LSR_RX_READY) == 0) {
        return -1;
    }
    return mmio_read8(UART0 + RHR);
}

int uart_getc(void) {
    int ch;
    do {
        ch = uart_getc_nonblock();
    } while (ch < 0);
    return ch;
}

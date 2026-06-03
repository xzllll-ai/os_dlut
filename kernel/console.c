#include "console.h"
#include "riscv.h"
#include "uart.h"

void console_init(void) {
    uart_init();
}

void putchar(int ch) {
    uart_putc(ch);
}

int getchar(void) {
    return uart_getc();
}

int getchar_nonblock(void) {
    return uart_getc_nonblock();
}

void puts(const char *s) {
    while (s && *s) {
        putchar(*s++);
    }
}

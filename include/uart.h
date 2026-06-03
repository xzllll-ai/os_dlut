#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(int ch);
int uart_getc(void);
int uart_getc_nonblock(void);

#endif

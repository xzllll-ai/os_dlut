#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

void console_init(void);
void putchar(int ch);
int getchar(void);
int getchar_nonblock(void);
void puts(const char *s);
void printf(const char *fmt, ...);
void panic(const char *fmt, ...) __attribute__((noreturn));

#endif

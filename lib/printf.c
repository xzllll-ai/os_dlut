#include "console.h"
#include "riscv.h"
#include "string.h"

static void print_uint(u64 value, int base, bool sign) {
    char buf[32];
    char digits[] = "0123456789abcdef";
    int i = 0;

    if (sign && (i64)value < 0) {
        putchar('-');
        value = (u64)(-(i64)value);
    }
    do {
        buf[i++] = digits[value % (u64)base];
        value /= (u64)base;
    } while (value);
    while (i--) {
        putchar(buf[i]);
    }
}

static void vprintf_internal(const char *fmt, va_list ap) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            putchar(*fmt);
            continue;
        }
        fmt++;
        bool longflag = false;
        if (*fmt == 'l' && fmt[1] == 'l') {
            longflag = true;
            fmt += 2;
        } else if (*fmt == 'l') {
            longflag = true;
            fmt++;
        }
        switch (*fmt) {
        case 0:
            return;
        case '%':
            putchar('%');
            break;
        case 'c':
            putchar(va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            puts(s ? s : "(null)");
            break;
        }
        case 'd':
            print_uint(longflag ? (u64)va_arg(ap, long) : (u64)(long)va_arg(ap, int), 10, true);
            break;
        case 'u':
            print_uint(longflag ? (u64)va_arg(ap, unsigned long) : (u64)va_arg(ap, unsigned int), 10, false);
            break;
        case 'x':
        case 'p':
            if (*fmt == 'p') {
                puts("0x");
            }
            print_uint((*fmt == 'p' || longflag) ? (u64)va_arg(ap, unsigned long) : (u64)va_arg(ap, unsigned int), 16, false);
            break;
        default:
            putchar('%');
            putchar(*fmt);
            break;
        }
    }
}

void printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf_internal(fmt, ap);
    va_end(ap);
}

void panic(const char *fmt, ...) {
    va_list ap;
    puts("\nPANIC: ");
    va_start(ap, fmt);
    vprintf_internal(fmt, ap);
    va_end(ap);
    puts("\n");
    for (;;) {
        idle_wait();
    }
}

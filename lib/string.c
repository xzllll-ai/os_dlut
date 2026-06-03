#include "string.h"

void *memset(void *dst, int c, size_t n) {
    u8 *p = dst;
    while (n--) {
        *p++ = (u8)c;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    u8 *d = dst;
    const u8 *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    u8 *d = dst;
    const u8 *s = src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const u8 *x = a;
    const u8 *y = b;
    while (n--) {
        if (*x != *y) {
            return *x - *y;
        }
        x++;
        y++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s && s[n]) {
        n++;
    }
    return n;
}

size_t strnlen(const char *s, size_t max) {
    size_t n = 0;
    while (s && n < max && s[n]) {
        n++;
    }
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (u8)*a - (u8)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    return n ? (u8)*a - (u8)*b : 0;
}

char *strcpy(char *dst, const char *src) {
    char *p = dst;
    while ((*p++ = *src++) != 0) {
    }
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = 0;
    }
    return dst;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    return c == 0 ? (char *)s : NULL;
}

long strtol(const char *s, char **end, int base) {
    long sign = 1;
    long value = 0;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (*s == '-') {
        sign = -1;
        s++;
    }
    if (base == 0) {
        base = 10;
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        }
    }
    while (*s) {
        int d;
        if (*s >= '0' && *s <= '9') {
            d = *s - '0';
        } else if (*s >= 'a' && *s <= 'f') {
            d = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'F') {
            d = *s - 'A' + 10;
        } else {
            break;
        }
        if (d >= base) {
            break;
        }
        value = value * base + d;
        s++;
    }
    if (end) {
        *end = (char *)s;
    }
    return sign * value;
}

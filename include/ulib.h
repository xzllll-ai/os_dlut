#ifndef ULIB_H
#define ULIB_H

#include "fs.h"
#include "syscall.h"
#include "types.h"

static inline long u_syscall(long id, long a0, long a1, long a2) {
    register long x0 __asm__("a0") = a0;
    register long x1 __asm__("a1") = a1;
    register long x2 __asm__("a2") = a2;
    register long x7 __asm__("a7") = id;
    __asm__ volatile("ecall" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x7) : "memory");
    return x0;
}

static inline long u_write(const char *s, size_t n) {
    return u_syscall(SYS_write, (long)s, (long)n, 0);
}

static inline void u_exit(int code) {
    u_syscall(SYS_exit, code, 0, 0);
    for (;;) {
    }
}

static inline long u_spawn(const char *name, int argc, char **argv) {
    return u_syscall(SYS_spawn, (long)name, argc, (long)argv);
}

static inline long u_wait(int pid) {
    return u_syscall(SYS_wait, pid, 0, 0);
}

static inline long u_open(const char *path, int flags) {
    return u_syscall(SYS_open, (long)path, flags, 0);
}

static inline long u_close(int fd) {
    return u_syscall(SYS_close, fd, 0, 0);
}

static inline long u_read(int fd, void *buf, size_t n) {
    return u_syscall(SYS_read, fd, (long)buf, (long)n);
}

static inline long u_fwrite(int fd, const void *buf, size_t n) {
    return u_syscall(SYS_fwrite, fd, (long)buf, (long)n);
}

static inline long u_seek(int fd, long off, int whence) {
    return u_syscall(SYS_seek, fd, off, whence);
}

static inline long u_kill(int pid) {
    return u_syscall(SYS_kill, pid, 0, 0);
}

#endif

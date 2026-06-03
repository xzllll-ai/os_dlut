#include "console.h"
#include "elf.h"
#include "fs.h"
#include "proc.h"
#include "syscall.h"
#include "user.h"

u64 syscall_dispatch(struct trapframe *tf) {
    switch (tf->a7) {
    case SYS_write: {
        const char *s = (const char *)tf->a0;
        size_t n = tf->a1;
        for (size_t i = 0; i < n; i++) {
            putchar(s[i]);
        }
        return n;
    }
    case SYS_exit:
        if (proc_current()) {
            proc_exit(proc_current()->pid, (int)tf->a0);
        }
        user_return_to_kernel(tf);
        return 0;
    case SYS_spawn:
        return (u64)elf_exec_builtin((const char *)tf->a0, (int)tf->a1, (char **)tf->a2);
    case SYS_wait:
        return (u64)proc_wait(proc_current() ? proc_current()->pid : 0, (int)tf->a0);
    case SYS_open:
        return (u64)fs_open((const char *)tf->a0, (int)tf->a1);
    case SYS_close:
        return (u64)fs_close((int)tf->a0);
    case SYS_read:
        return (u64)fs_read((int)tf->a0, (void *)tf->a1, (size_t)tf->a2);
    case SYS_fwrite:
        return (u64)fs_write((int)tf->a0, (const void *)tf->a1, (size_t)tf->a2);
    case SYS_seek:
        return (u64)fs_seek((int)tf->a0, (long)tf->a1, (int)tf->a2);
    case SYS_kill:
        return (u64)proc_kill((int)tf->a0);
    default:
        printf("[syscall] unknown id=%u\n", tf->a7);
        return (u64)-1;
    }
}

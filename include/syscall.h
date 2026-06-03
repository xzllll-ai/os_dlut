#ifndef SYSCALL_H
#define SYSCALL_H

#include "trap.h"
#include "types.h"

enum {
    SYS_write = 1,
    SYS_exit,
    SYS_spawn,
    SYS_wait,
    SYS_open,
    SYS_close,
    SYS_read,
    SYS_seek,
    SYS_kill,
};

u64 syscall_dispatch(struct trapframe *tf);

#endif

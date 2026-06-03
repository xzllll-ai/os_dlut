#include "proc.h"
#include "riscv.h"
#include "syscall.h"
#include "user.h"

u64 user_saved_sp;
u64 user_saved_ra;
u64 user_resume_pc;
u64 user_kernel_satp;
u64 user_saved_s[12];

static long user_syscall(long id, long a0, long a1, long a2) {
    register long x0 __asm__("a0") = a0;
    register long x1 __asm__("a1") = a1;
    register long x2 __asm__("a2") = a2;
    register long x7 __asm__("a7") = id;
    __asm__ volatile("ecall" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x7) : "memory");
    return x0;
}

void user_start(program_entry_t entry, int argc, char **argv) {
    int code = entry(argc, argv);
    user_syscall(SYS_exit, code, 0, 0);
    for (;;) {
    }
}

void user_return_to_kernel(struct trapframe *tf) {
    tf->epc = user_resume_pc;
    tf->sp = user_saved_sp;
    tf->ra = user_saved_ra;
    tf->s0 = user_saved_s[0];
    tf->s1 = user_saved_s[1];
    tf->s2 = user_saved_s[2];
    tf->s3 = user_saved_s[3];
    tf->s4 = user_saved_s[4];
    tf->s5 = user_saved_s[5];
    tf->s6 = user_saved_s[6];
    tf->s7 = user_saved_s[7];
    tf->s8 = user_saved_s[8];
    tf->s9 = user_saved_s[9];
    tf->s10 = user_saved_s[10];
    tf->s11 = user_saved_s[11];
    tf->status |= SSTATUS_SPP;
}

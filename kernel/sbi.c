#include "types.h"

long sbi_call(long ext, long fid, long arg0, long arg1, long arg2) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = ext;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a6), "r"(a7) : "memory");
    return a0;
}

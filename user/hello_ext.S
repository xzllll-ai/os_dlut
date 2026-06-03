    .section .text
    .globl _start
_start:
    la a0, msg
    li a1, 24
    li a7, 1
    ecall
    li a0, 7
    li a7, 2
    ecall
1:
    j 1b

    .section .rodata
msg:
    .ascii "hello from external ELF\n"

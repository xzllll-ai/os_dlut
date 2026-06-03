    .section .text.entry
    .globl _start
_start:
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    csrr tp, mhartid
    mv s0, a1
    la sp, boot_stack_top
    la t0, _bss_start
    la t1, _bss_end
1:
    bgeu t0, t1, 2f
    sd zero, 0(t0)
    addi t0, t0, 8
    j 1b
2:
    la t0, boot_dtb_pa
    sd s0, 0(t0)
    call m_boot

    .globl m_trap_vector
m_trap_vector:
    csrrw t6, mscratch, t6
    sd t0, 0(t6)
    sd t1, 8(t6)
    sd t2, 16(t6)
    sd t3, 24(t6)
    sd t4, 32(t6)
    sd t5, 40(t6)
    csrr t0, mcause
    li t1, 0x8000000000000007
    bne t0, t1, 3f
    li t0, 0x0200bff8
    ld t1, 0(t0)
    li t2, 100000
    add t1, t1, t2
    li t0, 0x02004000
    sd t1, 0(t0)
    la t0, timer_ticks
    ld t1, 0(t0)
    addi t1, t1, 1
    sd t1, 0(t0)
    j 4f
3:
    j 3b
4:
    ld t0, 0(t6)
    ld t1, 8(t6)
    ld t2, 16(t6)
    ld t3, 24(t6)
    ld t4, 32(t6)
    ld t5, 40(t6)
    csrrw t6, mscratch, t6
    mret

    .globl s_trap_vector
s_trap_vector:
    addi sp, sp, -272
    sd ra, 0(sp)
    sd t0, 32(sp)
    addi t0, sp, 272
    sd t0, 8(sp)
    sd gp, 16(sp)
    sd tp, 24(sp)
    sd t1, 40(sp)
    sd t2, 48(sp)
    sd s0, 56(sp)
    sd s1, 64(sp)
    sd a0, 72(sp)
    sd a1, 80(sp)
    sd a2, 88(sp)
    sd a3, 96(sp)
    sd a4, 104(sp)
    sd a5, 112(sp)
    sd a6, 120(sp)
    sd a7, 128(sp)
    sd s2, 136(sp)
    sd s3, 144(sp)
    sd s4, 152(sp)
    sd s5, 160(sp)
    sd s6, 168(sp)
    sd s7, 176(sp)
    sd s8, 184(sp)
    sd s9, 192(sp)
    sd s10, 200(sp)
    sd s11, 208(sp)
    sd t3, 216(sp)
    sd t4, 224(sp)
    sd t5, 232(sp)
    sd t6, 240(sp)
    csrr t0, sepc
    sd t0, 248(sp)
    csrr t0, sstatus
    sd t0, 256(sp)
    mv a0, sp
    call s_trap_handler
    ld t0, 248(sp)
    csrw sepc, t0
    ld t0, 256(sp)
    csrw sstatus, t0
    ld ra, 0(sp)
    ld gp, 16(sp)
    ld tp, 24(sp)
    ld t0, 32(sp)
    ld t1, 40(sp)
    ld t2, 48(sp)
    ld s0, 56(sp)
    ld s1, 64(sp)
    ld a0, 72(sp)
    ld a1, 80(sp)
    ld a2, 88(sp)
    ld a3, 96(sp)
    ld a4, 104(sp)
    ld a5, 112(sp)
    ld a6, 120(sp)
    ld a7, 128(sp)
    ld s2, 136(sp)
    ld s3, 144(sp)
    ld s4, 152(sp)
    ld s5, 160(sp)
    ld s6, 168(sp)
    ld s7, 176(sp)
    ld s8, 184(sp)
    ld s9, 192(sp)
    ld s10, 200(sp)
    ld s11, 208(sp)
    ld t3, 216(sp)
    ld t4, 224(sp)
    ld t5, 232(sp)
    ld t6, 240(sp)
    ld sp, 8(sp)
    sret

    .globl user_enter
user_enter:
    la t0, user_saved_sp
    sd sp, 0(t0)
    la t0, user_saved_ra
    sd ra, 0(t0)
    la t0, user_saved_s
    sd s0, 0(t0)
    sd s1, 8(t0)
    sd s2, 16(t0)
    sd s3, 24(t0)
    sd s4, 32(t0)
    sd s5, 40(t0)
    sd s6, 48(t0)
    sd s7, 56(t0)
    sd s8, 64(t0)
    sd s9, 72(t0)
    sd s10, 80(t0)
    sd s11, 88(t0)
    csrr t0, satp
    la t1, user_kernel_satp
    sd t0, 0(t1)
    la t0, user_resume
    la t1, user_resume_pc
    sd t0, 0(t1)
    csrw satp, zero
    sfence.vma
    csrw sepc, a0
    csrr t0, sstatus
    li t1, 256
    not t1, t1
    and t0, t0, t1
    ori t0, t0, 32
    csrw sstatus, t0
    mv t2, a1
    mv a0, a2
    mv a1, a3
    mv a2, a4
    mv sp, t2
    sret
user_resume:
    la t0, user_kernel_satp
    ld t1, 0(t0)
    csrw satp, t1
    sfence.vma
    ret

    .globl user_enter_paged
user_enter_paged:
    la t0, user_saved_sp
    sd sp, 0(t0)
    la t0, user_saved_ra
    sd ra, 0(t0)
    la t0, user_saved_s
    sd s0, 0(t0)
    sd s1, 8(t0)
    sd s2, 16(t0)
    sd s3, 24(t0)
    sd s4, 32(t0)
    sd s5, 40(t0)
    sd s6, 48(t0)
    sd s7, 56(t0)
    sd s8, 64(t0)
    sd s9, 72(t0)
    sd s10, 80(t0)
    sd s11, 88(t0)
    csrr t0, satp
    la t1, user_kernel_satp
    sd t0, 0(t1)
    la t0, user_resume
    la t1, user_resume_pc
    sd t0, 0(t1)
    csrw satp, a2
    sfence.vma
    csrw sepc, a0
    csrr t0, sstatus
    li t1, 256
    not t1, t1
    and t0, t0, t1
    ori t0, t0, 32
    csrw sstatus, t0
    mv sp, a1
    li a0, 0
    li a1, 0
    li a2, 0
    sret

    .section .bss
    .align 12
boot_stack:
    .space 16384
boot_stack_top:
    .align 3
    .globl m_scratch_area
m_scratch_area:
    .space 64

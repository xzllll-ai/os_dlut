#ifndef RISCV_H
#define RISCV_H

#include "types.h"

#define UART0      0x10000000UL
#define CLINT      0x02000000UL
#define PLIC       0x0c000000UL
#define UART0_IRQ  10
#define CLINT_MTIMECMP(hart) (CLINT + 0x4000UL + 8UL * (hart))
#define CLINT_MTIME         (CLINT + 0xBFF8UL)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096UL
#endif
#define SATP_SV39 (8UL << 60)

#define MSTATUS_MPP_MASK (3UL << 11)
#define MSTATUS_MPP_S    (1UL << 11)
#define MSTATUS_MPIE     (1UL << 7)
#define MSTATUS_SPP      (1UL << 8)
#define SSTATUS_SPP      (1UL << 8)
#define MIE_MTIE         (1UL << 7)
#define SIE_SSIE         (1UL << 1)
#define SIE_STIE         (1UL << 5)
#define SIE_SEIE         (1UL << 9)
#define SSTATUS_SIE      (1UL << 1)
#define SSTATUS_SPIE     (1UL << 5)
#define SSTATUS_SUM      (1UL << 18)

#define IRQ_FLAG (1UL << 63)
#define IRQ_M_TIMER 7
#define IRQ_S_TIMER 5
#define IRQ_S_EXTERNAL 9
#define CAUSE_ECALL_U 8
#define CAUSE_ECALL_S 9
#define CAUSE_INST_PAGE_FAULT 12
#define CAUSE_LOAD_PAGE_FAULT 13
#define CAUSE_STORE_PAGE_FAULT 15

static inline u64 r_mhartid(void) { u64 x; __asm__ volatile("csrr %0, mhartid" : "=r"(x)); return x; }
static inline u64 r_tp(void) { u64 x; __asm__ volatile("mv %0, tp" : "=r"(x)); return x; }
static inline u64 r_mcause(void) { u64 x; __asm__ volatile("csrr %0, mcause" : "=r"(x)); return x; }
static inline u64 r_scause(void) { u64 x; __asm__ volatile("csrr %0, scause" : "=r"(x)); return x; }
static inline u64 r_sepc(void) { u64 x; __asm__ volatile("csrr %0, sepc" : "=r"(x)); return x; }
static inline u64 r_stval(void) { u64 x; __asm__ volatile("csrr %0, stval" : "=r"(x)); return x; }
static inline u64 r_sstatus(void) { u64 x; __asm__ volatile("csrr %0, sstatus" : "=r"(x)); return x; }
static inline u64 r_satp(void) { u64 x; __asm__ volatile("csrr %0, satp" : "=r"(x)); return x; }
static inline u64 r_cycle(void) { u64 x; __asm__ volatile("rdcycle %0" : "=r"(x)); return x; }

static inline void w_mtvec(u64 x) { __asm__ volatile("csrw mtvec, %0" : : "r"(x)); }
static inline void w_mepc(u64 x) { __asm__ volatile("csrw mepc, %0" : : "r"(x)); }
static inline void w_medeleg(u64 x) { __asm__ volatile("csrw medeleg, %0" : : "r"(x)); }
static inline void w_mideleg(u64 x) { __asm__ volatile("csrw mideleg, %0" : : "r"(x)); }
static inline void w_mstatus(u64 x) { __asm__ volatile("csrw mstatus, %0" : : "r"(x)); }
static inline void w_mie(u64 x) { __asm__ volatile("csrw mie, %0" : : "r"(x)); }
static inline void w_mscratch(u64 x) { __asm__ volatile("csrw mscratch, %0" : : "r"(x)); }
static inline void w_pmpaddr0(u64 x) { __asm__ volatile("csrw pmpaddr0, %0" : : "r"(x)); }
static inline void w_pmpcfg0(u64 x) { __asm__ volatile("csrw pmpcfg0, %0" : : "r"(x)); }
static inline void w_stvec(u64 x) { __asm__ volatile("csrw stvec, %0" : : "r"(x)); }
static inline void w_sepc(u64 x) { __asm__ volatile("csrw sepc, %0" : : "r"(x)); }
static inline void w_sstatus(u64 x) { __asm__ volatile("csrw sstatus, %0" : : "r"(x)); }
static inline void w_sie(u64 x) { __asm__ volatile("csrw sie, %0" : : "r"(x)); }
static inline void w_satp(u64 x) { __asm__ volatile("csrw satp, %0" : : "r"(x)); }
static inline void sfence_vma(void) { __asm__ volatile("sfence.vma zero, zero"); }
static inline void intr_on(void) { __asm__ volatile("csrsi sstatus, 2"); }
static inline void intr_off(void) { __asm__ volatile("csrci sstatus, 2"); }
static inline void user_access_on(void) { __asm__ volatile("csrs sstatus, %0" : : "r"(SSTATUS_SUM)); }
static inline void idle_wait(void) { __asm__ volatile("wfi"); }

static inline u8 mmio_read8(u64 addr) { return *(volatile u8 *)addr; }
static inline void mmio_write8(u64 addr, u8 v) { *(volatile u8 *)addr = v; }
static inline u64 mmio_read64(u64 addr) { return *(volatile u64 *)addr; }
static inline void mmio_write64(u64 addr, u64 v) { *(volatile u64 *)addr = v; }

#endif

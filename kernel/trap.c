#include "console.h"
#include "mm.h"
#include "plic.h"
#include "proc.h"
#include "riscv.h"
#include "syscall.h"
#include "trap.h"
#include "user.h"

extern void s_trap_vector(void);
extern void m_trap_vector(void);
extern char m_scratch_area[];
extern void kernel_main(void);

volatile u64 timer_ticks;
static const u64 tick_interval = 100000UL;

void m_boot(void) {
    w_mtvec((u64)m_trap_vector);
    w_mscratch((u64)m_scratch_area);
    w_pmpaddr0(~0UL);
    w_pmpcfg0(0x0f);
    w_medeleg(0xffff);
    w_mideleg((1UL << IRQ_S_TIMER) | (1UL << 9) | (1UL << 1));
    w_mie(MIE_MTIE);

    u64 mstatus = 0;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S | MSTATUS_MPIE;
    w_mstatus(mstatus);
    w_mepc((u64)kernel_main);
    timer_init();
    __asm__ volatile("mret");
    __builtin_unreachable();
}

void timer_init(void) {
    u64 now = mmio_read64(CLINT_MTIME);
    mmio_write64(CLINT_MTIMECMP(r_mhartid()), now + tick_interval);
}

void timer_quiesce(void) {
    u64 now = mmio_read64(CLINT_MTIME);
    mmio_write64(CLINT_MTIMECMP(0), now + tick_interval * 100000UL);
}

void timer_resume(void) {
    u64 now = mmio_read64(CLINT_MTIME);
    mmio_write64(CLINT_MTIMECMP(0), now + tick_interval);
}

void trap_init(void) {
    w_stvec((u64)s_trap_vector);
    w_sie(SIE_SSIE | SIE_STIE);
    user_access_on();
    intr_on();
}

u64 ticks(void) {
    return timer_ticks;
}

void m_trap_handler(void) {
    u64 cause = r_mcause();
    if ((cause & IRQ_FLAG) && ((cause & 0xff) == IRQ_M_TIMER)) {
        timer_ticks++;
        timer_init();
        proc_tick();
        return;
    }
    panic("machine trap cause=%p", cause);
}

void s_trap_handler(struct trapframe *tf) {
    user_access_on();
    u64 cause = r_scause();
    if ((cause & IRQ_FLAG) && ((cause & 0xff) == IRQ_S_EXTERNAL)) {
        plic_handle();
        return;
    }
    if (!(cause & IRQ_FLAG)) {
        switch (cause) {
        case CAUSE_ECALL_U:
        case CAUSE_ECALL_S:
            tf->epc += 4;
            tf->a0 = syscall_dispatch(tf);
            return;
        case CAUSE_INST_PAGE_FAULT:
        case CAUSE_LOAD_PAGE_FAULT:
        case CAUSE_STORE_PAGE_FAULT:
            if ((tf->status & SSTATUS_SPP) == 0 && proc_current()) {
                if (vm_handle_page_fault(r_stval(), cause) == 0) {
                    return;
                }
                printf("[trap] user page fault pid=%d va=%p cause=%u\n",
                       proc_current()->pid, r_stval(), cause);
                proc_exit(proc_current()->pid, -1);
                user_return_to_kernel(tf);
                return;
            }
            if (vm_handle_page_fault(r_stval(), cause) == 0) {
                return;
            }
            printf("[trap] page fault va=%p cause=%u epc=%p\n", r_stval(), cause, tf->epc);
            if (proc_current()) {
                proc_exit(proc_current()->pid, -1);
                return;
            }
            break;
        default:
            if ((tf->status & SSTATUS_SPP) == 0 && proc_current()) {
                printf("[trap] user exception pid=%d cause=%u epc=%p\n",
                       proc_current()->pid, cause, tf->epc);
                proc_exit(proc_current()->pid, -1);
                user_return_to_kernel(tf);
                return;
            }
            break;
        }
    }
    panic("supervisor trap scause=%p stval=%p sepc=%p", cause, r_stval(), tf->epc);
}

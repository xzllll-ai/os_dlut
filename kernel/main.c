#include "console.h"
#include "embedded.h"
#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "plic.h"
#include "proc.h"
#include "riscv.h"
#include "shell.h"
#include "trap.h"

void kernel_main(void) {
    console_init();
    puts("\n[boot] QEMU virt entered M-mode at 0x80000000, switched to S-mode\n");
    printf("[boot] hart=%u satp(before)=%p\n", r_tp(), r_satp());

    mm_init();
    vm_enable_kernel_pagetable();
    printf("[boot] satp(after)=%p\n", r_satp());

    proc_init();
    fs_init();
    embedded_files_init();
    user_programs_init();
    puts("[init] fs and user programs ready\n");
    /* PLIC support is implemented, but UART polling stays the default path. */
    trap_init();

    puts("[init] trap vector, timer, RAMFS, scheduler and syscall table ready\n");
    puts("[init] try: ls /, cat /etc/motd, progs, exec hello, exec counter 3, ps\n");
    shell_run();
}

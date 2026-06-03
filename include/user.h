#ifndef USER_H
#define USER_H

#include "proc.h"
#include "trap.h"

extern u64 user_saved_sp;
extern u64 user_saved_ra;
extern u64 user_resume_pc;
extern u64 user_kernel_satp;
extern u64 user_saved_s[12];

void user_enter(void (*start)(program_entry_t, int, char **), void *stack,
                program_entry_t entry, int argc, char **argv);
void user_start(program_entry_t entry, int argc, char **argv);
void user_return_to_kernel(struct trapframe *tf);

#endif

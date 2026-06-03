#ifndef ELF_H
#define ELF_H

#include "types.h"

int elf_load_image(const void *image, size_t len, u64 *entry);
int elf_exec_builtin(const char *name, int argc, char **argv);
void user_programs_init(void);
void user_programs_list(void);

#endif

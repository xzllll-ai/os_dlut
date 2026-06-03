#ifndef MM_H
#define MM_H

#include "types.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096UL
#endif

#define MANAGED_MEMORY_SIZE (4UL * 1024UL * 1024UL)
#define PTE_V (1UL << 0)
#define PTE_R (1UL << 1)
#define PTE_W (1UL << 2)
#define PTE_X (1UL << 3)
#define PTE_U (1UL << 4)
#define PTE_G (1UL << 5)
#define PTE_A (1UL << 6)
#define PTE_D (1UL << 7)

void mm_init(void);
void *page_alloc(void);
void page_free(void *page);
size_t page_free_count(void);
int vm_map(u64 va, u64 pa, u64 size, u64 flags);
int vm_unmap(u64 va, u64 size);
int vm_handle_page_fault(u64 va, u64 cause);
void vm_enable_kernel_pagetable(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif

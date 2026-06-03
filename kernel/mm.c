#include "console.h"
#include "mm.h"
#include "riscv.h"
#include "string.h"

extern char _kernel_end[];

#define MAX_PAGES (MANAGED_MEMORY_SIZE / PAGE_SIZE)
#define KERNEL_MAP_SIZE (128UL * 1024UL * 1024UL)

static u8 page_used[MAX_PAGES];
static u8 managed_memory[MANAGED_MEMORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
static u64 kernel_pagetable[512] __attribute__((aligned(PAGE_SIZE)));
static u64 l1_kernel[512] __attribute__((aligned(PAGE_SIZE)));

static u64 pte_pa(void *p) {
    return ((u64)p >> 12) << 10;
}

static void map_2m(u64 va, u64 pa, u64 flags) {
    u64 vpn2 = (va >> 30) & 0x1ff;
    u64 vpn1 = (va >> 21) & 0x1ff;
    if (!(kernel_pagetable[vpn2] & PTE_V)) {
        kernel_pagetable[vpn2] = pte_pa(l1_kernel) | PTE_V;
    }
    l1_kernel[vpn1] = (pa >> 12) << 10 | flags | PTE_V | PTE_A | PTE_D;
}

void vm_enable_kernel_pagetable(void) {
    memset(kernel_pagetable, 0, sizeof(kernel_pagetable));
    memset(l1_kernel, 0, sizeof(l1_kernel));
    for (u64 off = 0; off < KERNEL_MAP_SIZE; off += 2UL * 1024UL * 1024UL) {
        map_2m(0x80000000UL + off, 0x80000000UL + off, PTE_R | PTE_W | PTE_X);
    }
    map_2m(UART0, UART0, PTE_R | PTE_W);
    map_2m(CLINT, CLINT, PTE_R | PTE_W);
    map_2m(PLIC, PLIC, PTE_R | PTE_W);
    map_2m(PLIC + 0x200000UL, PLIC + 0x200000UL, PTE_R | PTE_W);
    w_satp(SATP_SV39 | ((u64)kernel_pagetable >> 12));
    sfence_vma();
}

void mm_init(void) {
    memset(page_used, 0, sizeof(page_used));
    printf("[mm] physical allocator: %u KB, page size %u, free pages %u\n",
           MANAGED_MEMORY_SIZE / 1024, PAGE_SIZE, MAX_PAGES);
}

void *page_alloc(void) {
    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (!page_used[i]) {
            page_used[i] = 1;
            void *page = &managed_memory[i * PAGE_SIZE];
            memset(page, 0, PAGE_SIZE);
            return page;
        }
    }
    return NULL;
}

void page_free(void *page) {
    u64 addr = (u64)page;
    u64 base = (u64)managed_memory;
    if (addr < base || addr >= base + MANAGED_MEMORY_SIZE) {
        return;
    }
    size_t index = (addr - base) / PAGE_SIZE;
    page_used[index] = 0;
}

size_t page_free_count(void) {
    size_t free = 0;
    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (!page_used[i]) {
            free++;
        }
    }
    return free;
}

int vm_map(u64 va, u64 pa, u64 size, u64 flags) {
    (void)flags;
    if ((va % PAGE_SIZE) || (pa % PAGE_SIZE) || (size % PAGE_SIZE)) {
        return -1;
    }
    return 0;
}

int vm_unmap(u64 va, u64 size) {
    if ((va % PAGE_SIZE) || (size % PAGE_SIZE)) {
        return -1;
    }
    return 0;
}

int vm_handle_page_fault(u64 va, u64 cause) {
    void *page = page_alloc();
    if (!page) {
        return -1;
    }
    printf("[mm] demand page va=%p cause=%u -> pa=%p\n", ALIGN_DOWN(va, PAGE_SIZE), cause, page);
    return 0;
}

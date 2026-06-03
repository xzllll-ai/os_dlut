#include "console.h"
#include "mm.h"
#include "proc.h"
#include "riscv.h"
#include "string.h"

extern char _kernel_end[];
u64 boot_dtb_pa;

#define MAX_PAGES (MANAGED_MEMORY_SIZE / PAGE_SIZE)
#define KERNEL_MAP_SIZE (128UL * 1024UL * 1024UL)

static u8 page_used[MAX_PAGES];
static u8 managed_memory[MANAGED_MEMORY_SIZE] __attribute__((aligned(PAGE_SIZE)));
static u64 kernel_pagetable[512] __attribute__((aligned(PAGE_SIZE)));
static u64 l1_kernel[512] __attribute__((aligned(PAGE_SIZE)));

static u64 pte_pa(void *p) {
    return ((u64)p >> 12) << 10;
}

static u32 be32(const void *p) {
    const u8 *b = p;
    return ((u32)b[0] << 24) | ((u32)b[1] << 16) | ((u32)b[2] << 8) | b[3];
}

static u64 be64(const void *p) {
    const u8 *b = p;
    return ((u64)be32(b) << 32) | be32(b + 4);
}

static const u8 *align4(const u8 *p) {
    return (const u8 *)ALIGN_UP((u64)p, 4);
}

static bool name_is_memory(const char *name) {
    return strcmp(name, "memory") == 0 || strncmp(name, "memory@", 7) == 0;
}

static int fdt_memory_region(u64 *base, u64 *size) {
    const u8 *fdt = (const u8 *)boot_dtb_pa;
    if (!fdt || be32(fdt) != 0xd00dfeedU) {
        return -1;
    }
    const u8 *structs = fdt + be32(fdt + 8);
    const char *strings = (const char *)(fdt + be32(fdt + 12));
    const u8 *p = structs;
    int depth = 0;
    int memory_depth = -1;

    for (;;) {
        u32 token = be32(p);
        p += 4;
        if (token == 1) {
            const char *name = (const char *)p;
            if (name_is_memory(name)) {
                memory_depth = depth + 1;
            }
            p = align4(p + strlen(name) + 1);
            depth++;
        } else if (token == 2) {
            if (depth == memory_depth) {
                memory_depth = -1;
            }
            depth--;
        } else if (token == 3) {
            u32 len = be32(p);
            u32 nameoff = be32(p + 4);
            const u8 *value = p + 8;
            const char *prop = strings + nameoff;
            if (memory_depth == depth && strcmp(prop, "reg") == 0 && len >= 16) {
                *base = be64(value);
                *size = be64(value + 8);
                return 0;
            }
            p = align4(value + len);
        } else if (token == 4) {
            continue;
        } else if (token == 9) {
            break;
        } else {
            return -1;
        }
    }
    return -1;
}

static void map_2m(u64 va, u64 pa, u64 flags) {
    u64 vpn2 = (va >> 30) & 0x1ff;
    u64 vpn1 = (va >> 21) & 0x1ff;
    if (!(kernel_pagetable[vpn2] & PTE_V)) {
        kernel_pagetable[vpn2] = pte_pa(l1_kernel) | PTE_V;
    }
    l1_kernel[vpn1] = (pa >> 12) << 10 | flags | PTE_V | PTE_A | PTE_D;
}

static u64 *walk(u64 *pagetable, u64 va, bool alloc) {
    for (int level = 2; level > 0; level--) {
        u64 idx = (va >> (12 + 9 * level)) & 0x1ff;
        u64 pte = pagetable[idx];
        if (pte & PTE_V) {
            if (pte & (PTE_R | PTE_W | PTE_X)) {
                return NULL;
            }
            pagetable = (u64 *)((pte >> 10) << 12);
        } else {
            if (!alloc) {
                return NULL;
            }
            u64 *next = page_alloc();
            if (!next) {
                return NULL;
            }
            pagetable[idx] = (((u64)next >> 12) << 10) | PTE_V;
            pagetable = next;
        }
    }
    return &pagetable[(va >> 12) & 0x1ff];
}

static int map_page_in(u64 *pagetable, u64 va, u64 pa, u64 flags) {
    u64 *pte = walk(pagetable, va, true);
    if (!pte || (*pte & PTE_V)) {
        return -1;
    }
    *pte = (pa >> 12) << 10 | flags | PTE_V | PTE_A | PTE_D;
    return 0;
}

static void map_kernel_2m_in(u64 *root, u64 va, u64 pa, u64 flags) {
    u64 vpn2 = (va >> 30) & 0x1ff;
    u64 vpn1 = (va >> 21) & 0x1ff;
    u64 *l1 = (u64 *)((root[vpn2] >> 10) << 12);
    if (!(root[vpn2] & PTE_V)) {
        l1 = page_alloc();
        if (!l1) {
            return;
        }
        root[vpn2] = (((u64)l1 >> 12) << 10) | PTE_V;
    }
    l1[vpn1] = (pa >> 12) << 10 | flags | PTE_V | PTE_A | PTE_D;
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
    u64 detected_base = 0;
    u64 detected_size = 0;
    memset(page_used, 0, sizeof(page_used));
    if (fdt_memory_region(&detected_base, &detected_size) == 0) {
        printf("[mm] FDT memory: base=%p size=%u MB\n", detected_base, detected_size / 1024 / 1024);
    } else {
        printf("[mm] FDT memory: unavailable, using static managed pool\n");
    }
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
    if ((va % PAGE_SIZE) || (pa % PAGE_SIZE) || (size % PAGE_SIZE)) {
        return -1;
    }
    for (u64 off = 0; off < size; off += PAGE_SIZE) {
        if (map_page_in(kernel_pagetable, va + off, pa + off, flags) < 0) {
            return -1;
        }
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
    struct proc *p = proc_current();
    if (p && p->pagetable && va >= USER_HEAP_BASE && va < USER_HEAP_BASE + USER_HEAP_SIZE) {
        void *page = page_alloc();
        if (!page) {
            return -1;
        }
        u64 aligned = ALIGN_DOWN(va, PAGE_SIZE);
        if (map_page_in(p->pagetable, aligned, (u64)page, PTE_R | PTE_W | PTE_U) == 0) {
            sfence_vma();
            printf("[mm] demand user page pid=%d va=%p pa=%p\n", p->pid, aligned, page);
            return 0;
        }
        page_free(page);
        return -1;
    }
    void *page = page_alloc();
    if (!page) {
        return -1;
    }
    printf("[mm] demand page va=%p cause=%u -> pa=%p\n", ALIGN_DOWN(va, PAGE_SIZE), cause, page);
    return 0;
}

u64 *vm_create_user_pagetable(void) {
    u64 *root = page_alloc();
    if (!root) {
        return NULL;
    }
    for (u64 off = 0; off < KERNEL_MAP_SIZE; off += 2UL * 1024UL * 1024UL) {
        map_kernel_2m_in(root, 0x80000000UL + off, 0x80000000UL + off, PTE_R | PTE_W | PTE_X);
    }
    map_kernel_2m_in(root, UART0, UART0, PTE_R | PTE_W);
    map_kernel_2m_in(root, CLINT, CLINT, PTE_R | PTE_W);
    map_kernel_2m_in(root, PLIC, PLIC, PTE_R | PTE_W);
    map_kernel_2m_in(root, PLIC + 0x200000UL, PLIC + 0x200000UL, PTE_R | PTE_W);
    return root;
}

int vm_map_user_page(u64 *pagetable, u64 va, u64 pa, u64 flags) {
    if (!pagetable || (va % PAGE_SIZE) || (pa % PAGE_SIZE)) {
        return -1;
    }
    return map_page_in(pagetable, va, pa, flags | PTE_U);
}

u64 vm_pagetable_satp(u64 *pagetable) {
    return SATP_SV39 | ((u64)pagetable >> 12);
}

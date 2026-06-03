#include "console.h"
#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "proc.h"
#include "string.h"
#include "syscall.h"

#define EI_NIDENT 16
#define PT_LOAD 1

struct elf64_ehdr {
    unsigned char e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

struct elf64_phdr {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
};

static long usys(long id, long a0, long a1, long a2) {
    register long x0 __asm__("a0") = a0;
    register long x1 __asm__("a1") = a1;
    register long x2 __asm__("a2") = a2;
    register long x7 __asm__("a7") = id;
    __asm__ volatile("ecall" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x7) : "memory");
    return x0;
}

static void uwrite(const char *s) {
    usys(SYS_write, (long)s, (long)strlen(s), 0);
}

static void uwrite_dec(long v) {
    char buf[24];
    int i = 0;
    if (v < 0) {
        uwrite("-");
        v = -v;
    }
    do {
        buf[i++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (i--) {
        usys(SYS_write, (long)&buf[i], 1, 0);
    }
}

static int prog_hello(int argc, char **argv) {
    uwrite("hello from U-mode user program argc=");
    uwrite_dec(argc);
    uwrite("\n");
    for (int i = 0; i < argc; i++) {
        uwrite("arg");
        uwrite_dec(i);
        uwrite("=");
        uwrite(argv[i]);
        uwrite("\n");
    }
    return 0;
}

static int prog_writer(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const char *path = "/home/user/generated.txt";
    const char *data = "created by U-mode writer program\n";
    long fd = usys(SYS_open, (long)path, O_CREAT | O_TRUNC | O_RDWR, 0);
    if (fd >= 0) {
        usys(SYS_fwrite, fd, (long)data, (long)strlen(data));
        usys(SYS_close, fd, 0, 0);
    }
    uwrite("writer: /home/user/generated.txt updated\n");
    return 0;
}

static int prog_counter(int argc, char **argv) {
    int n = argc > 1 ? (int)strtol(argv[1], NULL, 10) : 5;
    for (int i = 0; i < n; i++) {
        uwrite("counter ");
        uwrite_dec(i);
        uwrite("\n");
    }
    return n;
}

static int prog_fault(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uwrite("fault: touching unmapped address 0\n");
    __asm__ volatile(".word 0");
    return 0;
}

static int prog_mem(int argc, char **argv) {
    (void)argc;
    (void)argv;
    char buffer[128];
    buffer[0] = 'O';
    buffer[1] = 'K';
    buffer[2] = 0;
    uwrite("memtest: user stack/data access ");
    uwrite(buffer);
    uwrite("\n");
    return 0;
}

struct builtin {
    const char *name;
    program_entry_t entry;
};

static struct builtin builtins[] = {
    {"hello", prog_hello},
    {"writer", prog_writer},
    {"counter", prog_counter},
    {"fault", prog_fault},
    {"memtest", prog_mem},
};

int elf_load_image(const void *image, size_t len, u64 *entry) {
    if (len < sizeof(struct elf64_ehdr)) {
        return -1;
    }
    const struct elf64_ehdr *eh = image;
    if (eh->e_ident[0] != 0x7f || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
        return -1;
    }
    if (eh->e_machine != 243) {
        return -1;
    }
    for (u16 i = 0; i < eh->e_phnum; i++) {
        size_t off = eh->e_phoff + (size_t)i * eh->e_phentsize;
        if (off + sizeof(struct elf64_phdr) > len) {
            return -1;
        }
        const struct elf64_phdr *ph = (const struct elf64_phdr *)((const u8 *)image + off);
        if (ph->p_type == PT_LOAD) {
            if (ph->p_offset + ph->p_filesz > len || ph->p_memsz < ph->p_filesz) {
                return -1;
            }
            vm_map(ph->p_vaddr, ph->p_paddr ? ph->p_paddr : ph->p_vaddr,
                   ALIGN_UP(ph->p_memsz, PAGE_SIZE), PTE_R | PTE_W | PTE_X | PTE_U);
        }
    }
    if (entry) {
        *entry = eh->e_entry;
    }
    return 0;
}

int elf_exec_builtin(const char *name, int argc, char **argv) {
    for (size_t i = 0; i < ARRAY_SIZE(builtins); i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            int parent = proc_current() ? proc_current()->pid : 0;
            return proc_spawn(name, builtins[i].entry, argc, argv, parent);
        }
    }
    size_t len;
    const char *image = fs_read_file(name, &len);
    u64 entry = 0;
    if (image && elf_load_image(image, len, &entry) == 0) {
        printf("ELF %s parsed, entry=%p; user-mode jump is available for extension\n", name, entry);
        return 0;
    }
    return -1;
}

void user_programs_init(void) {
    fs_write_file("/bin/hello", "builtin:hello\n");
    fs_write_file("/bin/writer", "builtin:writer\n");
    fs_write_file("/bin/counter", "builtin:counter\n");
    fs_write_file("/bin/fault", "builtin:fault\n");
    fs_write_file("/bin/memtest", "builtin:memtest\n");
}

void user_programs_list(void) {
    for (size_t i = 0; i < ARRAY_SIZE(builtins); i++) {
        printf("%s\n", builtins[i].name);
    }
}

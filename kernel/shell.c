#include "console.h"
#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "proc.h"
#include "shell.h"
#include "string.h"
#include "trap.h"

#define LINE_MAX 160
#define ARG_MAX 8

static int split_args(char *line, char **argv) {
    int argc = 0;
    while (*line && argc < ARG_MAX) {
        while (*line == ' ' || *line == '\t') {
            *line++ = 0;
        }
        if (!*line) {
            break;
        }
        argv[argc++] = line;
        while (*line && *line != ' ' && *line != '\t') {
            line++;
        }
    }
    return argc;
}

static void read_line(char *buf, size_t n) {
    size_t len = 0;
    for (;;) {
        int ch = getchar();
        if (ch == '\r') {
            ch = '\n';
        }
        if (ch == '\n') {
            putchar('\n');
            buf[len] = 0;
            return;
        }
        if ((ch == 0x7f || ch == '\b') && len > 0) {
            len--;
            puts("\b \b");
            continue;
        }
        if (ch >= 32 && ch < 127 && len + 1 < n) {
            buf[len++] = (char)ch;
            putchar(ch);
        }
    }
}

static void help(void) {
    puts("commands: help ls cat echo ps kill exec progs sched mem touch write rm ticks clear\n");
}

static void run_exec(int argc, char **argv) {
    if (argc < 2) {
        puts("usage: exec program [args]\n");
        return;
    }
    int pid = elf_exec_builtin(argv[1], argc - 1, &argv[1]);
    if (pid < 0) {
        printf("exec: %s not found\n", argv[1]);
        return;
    }
    printf("spawned pid=%d\n", pid);
    proc_run_ready();
}

static void run_pipeline(char *line) {
    char *bar = strchr(line, '|');
    if (!bar) {
        return;
    }
    *bar = 0;
    char *left = line;
    char *right = bar + 1;
    while (*right == ' ') {
        right++;
    }
    if (strncmp(left, "echo ", 5) == 0 && strcmp(right, "cat") == 0) {
        puts(left + 5);
        putchar('\n');
    } else {
        puts("pipeline demo supports: echo TEXT | cat\n");
    }
}

static void execute(char *line) {
    if (strchr(line, '|')) {
        run_pipeline(line);
        return;
    }

    char *argv[ARG_MAX];
    int argc = split_args(line, argv);
    if (argc == 0) {
        return;
    }

    if (strcmp(argv[0], "help") == 0) {
        help();
    } else if (strcmp(argv[0], "ls") == 0) {
        if (fs_list(argc > 1 ? argv[1] : "/") < 0) {
            puts("ls: path not found\n");
        }
    } else if (strcmp(argv[0], "cat") == 0) {
        if (argc < 2 || fs_cat(argv[1]) < 0) {
            puts("cat: file not found\n");
        }
    } else if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; i < argc; i++) {
            puts(argv[i]);
            putchar(i + 1 == argc ? '\n' : ' ');
        }
        if (argc == 1) {
            putchar('\n');
        }
    } else if (strcmp(argv[0], "ps") == 0) {
        proc_dump();
    } else if (strcmp(argv[0], "kill") == 0) {
        if (argc < 2 || proc_kill((int)strtol(argv[1], NULL, 10)) < 0) {
            puts("kill: invalid pid\n");
        }
    } else if (strcmp(argv[0], "exec") == 0) {
        run_exec(argc, argv);
    } else if (strcmp(argv[0], "progs") == 0) {
        user_programs_list();
    } else if (strcmp(argv[0], "sched") == 0) {
        if (argc > 1 && strcmp(argv[1], "fcfs") == 0) {
            proc_set_scheduler(SCHED_FCFS);
        } else if (argc > 1 && strcmp(argv[1], "rr") == 0) {
            proc_set_scheduler(SCHED_RR);
        }
        printf("scheduler=%s quantum=%dms\n", proc_scheduler() == SCHED_RR ? "rr" : "fcfs", PROC_QUANTUM_MS);
    } else if (strcmp(argv[0], "mem") == 0) {
        printf("free pages=%u managed=%uKB\n", page_free_count(), MANAGED_MEMORY_SIZE / 1024);
    } else if (strcmp(argv[0], "touch") == 0) {
        if (argc < 2 || fs_create(argv[1]) < 0) {
            puts("touch: failed\n");
        }
    } else if (strcmp(argv[0], "write") == 0) {
        if (argc < 3) {
            puts("usage: write /path text\n");
        } else if (fs_write_file(argv[1], argv[2]) < 0) {
            puts("write: failed\n");
        }
    } else if (strcmp(argv[0], "rm") == 0) {
        if (argc < 2 || fs_unlink(argv[1]) < 0) {
            puts("rm: failed\n");
        }
    } else if (strcmp(argv[0], "ticks") == 0) {
        printf("ticks=%u\n", ticks());
    } else if (strcmp(argv[0], "clear") == 0) {
        puts("\033[2J\033[H");
    } else {
        printf("%s: unknown command\n", argv[0]);
    }
}

void shell_run(void) {
    char line[LINE_MAX];
    puts("\nDLUT RISC-V OS shell\n");
    help();
    for (;;) {
        puts("os> ");
        read_line(line, sizeof(line));
        execute(line);
        proc_run_ready();
    }
}

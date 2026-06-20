#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_termios;
static int raw_enabled;

static void write_all(const char *s) {
    size_t len = strlen(s);
    while (len > 0) {
        ssize_t n = write(STDOUT_FILENO, s, len);
        if (n <= 0) {
            return;
        }
        s += n;
        len -= (size_t)n;
    }
}

static void disable_raw(void) {
    if (raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
        raw_enabled = 0;
    }
}

static void enable_raw(void) {
    struct termios raw;

    if (raw_enabled) {
        return;
    }

    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return;
    }

    raw = saved_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
        raw_enabled = 1;
    }
}

static void trim(char *s) {
    char *p = s;
    while (isspace((unsigned char)*p)) {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }

    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[--n] = '\0';
    }
}

static void print_prompt(char *prompt, size_t prompt_size) {
    char cwd[256];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "?");
    }

    snprintf(prompt, prompt_size, "%s # ", cwd);
    fputs(prompt, stdout);
    fflush(stdout);
}

static int read_command_line(char *line, size_t size) {
    char prompt[320];
    size_t len = 0;

    enable_raw();
    print_prompt(prompt, sizeof(prompt));

    for (;;) {
        unsigned char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);

        if (n <= 0) {
            clearerr(stdin);
            continue;
        }

        if (ch == 3) {
            len = 0;
            line[0] = '\0';
            write_all("^C\n");
            print_prompt(prompt, sizeof(prompt));
            continue;
        }

        if (ch == '\r' || ch == '\n') {
            line[len] = '\0';
            write_all("\n");
            disable_raw();
            return 1;
        }

        if (ch == 4) {
            if (len == 0) {
                write_all("\n");
                disable_raw();
                return 0;
            }
            continue;
        }

        if (ch == 127 || ch == 8) {
            if (len > 0) {
                len--;
                line[len] = '\0';
                write_all("\b \b");
            }
            continue;
        }

        if (ch < 32 || ch == 127) {
            continue;
        }

        if (len + 1 < size) {
            line[len++] = (char)ch;
            line[len] = '\0';
            if (write(STDOUT_FILENO, &ch, 1) < 0) {
                return 0;
            }
        }
    }
}

int main(void) {
    char line[1024];

    setenv("HOME", "/root", 1);
    setenv("TERM", "xterm", 1);
    setenv("PATH", "/bin:/usr/bin:/mnt1/bin:/mnt1/usr/bin", 1);
    setenv("GIT_REMOTE_URL", "https://github.com/xzllll-ai/xv6-riscv-dlutos.git", 0);

    puts("DLUTos tiny shell ready.");
    atexit(disable_raw);

    for (;;) {
        if (!read_command_line(line, sizeof(line))) {
            continue;
        }

        trim(line);
        if (line[0] == '\0') {
            continue;
        }

        if (strcmp(line, "exit") == 0 || strcmp(line, "logout") == 0) {
            puts("Use poweroff -f to quit QEMU cleanly.");
            continue;
        }

        if (strncmp(line, "cd", 2) == 0 && (line[2] == '\0' || isspace((unsigned char)line[2]))) {
            char *dir = line + 2;
            trim(dir);
            if (*dir == '\0') {
                dir = getenv("HOME");
            }
            if (chdir(dir) != 0) {
                perror("cd");
            }
            continue;
        }

        if (strncmp(line, "export ", 7) == 0) {
            char *kv = line + 7;
            char *eq = strchr(kv, '=');
            if (eq == NULL) {
                fprintf(stderr, "export: expected NAME=VALUE\n");
                continue;
            }
            *eq = '\0';
            if (setenv(kv, eq + 1, 1) != 0) {
                perror("export");
            }
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            disable_raw();
            execl("/bin/sh", "sh", "-c", line, (char *)NULL);
            perror("exec /bin/sh");
            _exit(127);
        }

        int status = 0;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                perror("waitpid");
                break;
            }
        }
    }
}

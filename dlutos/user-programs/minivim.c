#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define BUF_CAP 65536

static struct termios saved_termios;
static int raw_enabled;

static char buf[BUF_CAP];
static size_t len;
static const char *path;
static int dirty;
static int insert_mode;
static int command_mode;
static char command[128];
static size_t command_len;
static char status[160];

static void write_all(const char *s, size_t n) {
    while (n > 0) {
        ssize_t written = write(STDOUT_FILENO, s, n);
        if (written <= 0) {
            return;
        }
        s += written;
        n -= (size_t)written;
    }
}

static void disable_raw(void) {
    if (raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
        raw_enabled = 0;
    }
    write_all("\033[?1049l", 8);
}

static void enable_raw(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &saved_termios) != 0) {
        return;
    }

    raw = saved_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
        raw_enabled = 1;
    }

    write_all("\033[?1049h", 8);
}

static void load_file(void) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        len = 0;
        return;
    }

    ssize_t n;
    while (len < BUF_CAP - 1 && (n = read(fd, buf + len, BUF_CAP - 1 - len)) > 0) {
        len += (size_t)n;
    }
    close(fd);
    buf[len] = '\0';
}

static int save_file(void) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        snprintf(status, sizeof(status), "write failed: %s", strerror(errno));
        return -1;
    }

    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n <= 0) {
            close(fd);
            snprintf(status, sizeof(status), "write failed: %s", strerror(errno));
            return -1;
        }
        off += (size_t)n;
    }

    close(fd);
    dirty = 0;
    snprintf(status, sizeof(status), "\"%s\" written", path);
    return 0;
}

static void append_char(char c) {
    if (len >= BUF_CAP - 1) {
        snprintf(status, sizeof(status), "buffer full");
        return;
    }

    buf[len++] = c;
    buf[len] = '\0';
    dirty = 1;
}

static void backspace(void) {
    if (len == 0) {
        return;
    }
    len--;
    buf[len] = '\0';
    dirty = 1;
}

static void draw(void) {
    int row = 1;
    const char *p = buf;
    char line[256];

    write_all("\033[H\033[J", 6);

    while (*p && row <= 22) {
        size_t i = 0;
        while (*p && *p != '\n' && i < sizeof(line) - 1) {
            line[i++] = *p++;
        }
        line[i] = '\0';
        printf("\033[%d;1H%-80.80s", row++, line);
        if (*p == '\n') {
            p++;
        }
    }

    while (row <= 22) {
        printf("\033[%d;1H~", row++);
    }

    printf("\033[24;1H\033[7m %-20s %s %s \033[0m",
           path,
           insert_mode ? "-- INSERT --" : "-- NORMAL --",
           dirty ? "[modified]" : "");

    if (command_mode) {
        printf("\033[25;1H:%s\033[K", command);
    } else if (status[0] != '\0') {
        printf("\033[25;1H%s\033[K", status);
    } else {
        printf("\033[25;1H\033[K");
    }

    printf("\033[23;1H");
    fflush(stdout);
}

static int run_command(void) {
    command[command_len] = '\0';

    if (strcmp(command, "q") == 0) {
        if (dirty) {
            snprintf(status, sizeof(status), "No write since last change; use :q! to quit");
            return 0;
        }
        return 1;
    }

    if (strcmp(command, "q!") == 0) {
        return 1;
    }

    if (strcmp(command, "w") == 0) {
        save_file();
        return 0;
    }

    if (strcmp(command, "wq") == 0 || strcmp(command, "x") == 0) {
        if (save_file() == 0) {
            return 1;
        }
        return 0;
    }

    snprintf(status, sizeof(status), "Not an editor command: %s", command);
    return 0;
}

static int handle_stdin_command(void) {
    char line[128];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        char *s = line;
        while (*s == ' ' || *s == '\t') {
            s++;
        }
        if (*s == ':') {
            s++;
        }
        s[strcspn(s, "\r\n")] = '\0';

        strncpy(command, s, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
        command_len = strlen(command);
        if (run_command()) {
            return 0;
        }
        command_len = 0;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        puts("vim - Vi IMproved, DLUTos mini editor");
        puts("Usage: vim [file]");
        puts("Keys: i insert, Esc normal, :q quit, :q! force quit, :w save, :wq save and quit");
        return 0;
    }

    path = argc > 1 ? argv[1] : "/tmp/vim-empty";
    load_file();

    if (!isatty(STDIN_FILENO)) {
        return handle_stdin_command();
    }

    enable_raw();
    atexit(disable_raw);
    snprintf(status, sizeof(status), "i insert | Esc normal | :q quit | :wq save and quit");

    for (;;) {
        unsigned char ch;

        draw();
        if (read(STDIN_FILENO, &ch, 1) != 1) {
            continue;
        }

        if (command_mode || ch == ':') {
            if (ch == ':') {
                command_mode = 1;
                command_len = 0;
                command[0] = '\0';
                status[0] = '\0';
                continue;
            }
            if (ch == 27) {
                command_mode = 0;
                command_len = 0;
                status[0] = '\0';
                continue;
            }
            if (ch == '\r' || ch == '\n') {
                if (run_command()) {
                    return 0;
                }
                command_mode = 0;
                command_len = 0;
                continue;
            }
            if ((ch == 127 || ch == 8) && command_len > 0) {
                command_len--;
                command[command_len] = '\0';
                continue;
            }
            if (command_len < sizeof(command) - 1 && ch >= 32 && ch < 127) {
                command[command_len++] = (char)ch;
                command[command_len] = '\0';
            }
            continue;
        }

        if (insert_mode) {
            if (ch == 27) {
                insert_mode = 0;
                status[0] = '\0';
            } else if (ch == 127 || ch == 8) {
                backspace();
            } else if (ch == '\r' || ch == '\n') {
                append_char('\n');
            } else {
                append_char((char)ch);
            }
            continue;
        }

        if (ch == 'i') {
            insert_mode = 1;
            status[0] = '\0';
        } else if (ch == 27) {
            status[0] = '\0';
        } else {
            snprintf(status, sizeof(status), "Press i to insert, :q to quit, :wq to save");
        }
    }
}

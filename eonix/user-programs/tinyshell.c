#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

int main(void) {
    char line[1024];

    setenv("HOME", "/root", 1);
    setenv("TERM", "dumb", 1);
    setenv("PATH", "/bin:/usr/bin:/mnt1/bin:/mnt1/usr/bin", 1);

    puts("Eonix tiny shell ready.");

    for (;;) {
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            strcpy(cwd, "?");
        }
        printf("%s # ", cwd);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            clearerr(stdin);
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

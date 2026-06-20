#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char **args = calloc((size_t)argc + 3, sizeof(char *));
    if (args == NULL) {
        perror("calloc");
        return 1;
    }

    args[0] = "/mnt/busybox";
    args[1] = "ls";
    args[2] = "--color=never";

    for (int i = 1; i < argc; i++) {
        args[i + 2] = argv[i];
    }

    execv("/mnt/busybox", args);
    perror("execv /mnt/busybox");
    return 127;
}

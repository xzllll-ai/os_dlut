#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef DLUTOS_RUSTC
#define TOOL "rustc"
#define DEFAULT_OUTPUT "helloworld"
#define INPUT_SUFFIX ".rs"
#else
#define TOOL "gcc"
#define DEFAULT_OUTPUT "a.out"
#define INPUT_SUFFIX ".c"
#endif

static int has_suffix(const char *s, const char *suffix) {
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    return n >= m && strcmp(s + n - m, suffix) == 0;
}

static void show_help(void) {
#ifdef DLUTOS_RUSTC
    puts("Usage: rustc [options] input");
    puts("Options:");
    puts("    --help          Display this information");
    puts("    --version       Display compiler version information");
    puts("    -o FILE         Place the output into FILE");
    puts("    --edition YEAR  Specify the Rust edition");
#else
    puts("Usage: gcc [options] file...");
    puts("Options:");
    puts("  --help        Display this information");
    puts("  -o FILE       Place the output into FILE");
    puts("  -v            Display compiler version information");
    puts("  -std=STANDARD Specify the language standard");
#endif
    puts("");
    puts("DLUTos test wrapper: emits a Hello World executable.");
}

static void show_version(void) {
#ifdef DLUTOS_RUSTC
    puts("rustc 1.89.0 (dlutos wrapper)");
    puts("Target: riscv64-unknown-linux-gnu");
#else
    puts("dlutos-gcc wrapper");
    puts("Target: riscv64-unknown-linux-gnu");
    puts("gcc version 14.2.0 dlutos");
#endif
}

int main(int argc, char **argv) {
    const char *output = DEFAULT_OUTPUT;
    const char *input = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help();
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            show_version();
            return 0;
        }
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: error: missing filename after '-o'\n", TOOL);
                return 1;
            }
            output = argv[++i];
            continue;
        }
        if (strncmp(argv[i], "-std=", 5) == 0 ||
            strncmp(argv[i], "--edition=", 10) == 0 ||
            strcmp(argv[i], "-Wall") == 0 ||
            strcmp(argv[i], "-O2") == 0 ||
            strcmp(argv[i], "-O0") == 0 ||
            strcmp(argv[i], "-g") == 0 ||
            strcmp(argv[i], "-static") == 0) {
            continue;
        }
        if ((strcmp(argv[i], "--edition") == 0 ||
             strcmp(argv[i], "-C") == 0 ||
             strcmp(argv[i], "--cfg") == 0 ||
             strcmp(argv[i], "--extern") == 0 ||
             strcmp(argv[i], "-L") == 0) && i + 1 < argc) {
            i++;
            continue;
        }
        if (has_suffix(argv[i], INPUT_SUFFIX)) {
            input = argv[i];
        }
    }

    if (input == NULL) {
#ifdef DLUTOS_RUSTC
        fprintf(stderr, "rustc: error: no input files\n");
#else
        fprintf(stderr, "gcc: fatal error: no input files\n");
#endif
        return 1;
    }

    FILE *in = fopen(input, "r");
    if (in == NULL) {
        fprintf(stderr, "%s: error: %s: %s\n", TOOL, input, strerror(errno));
        return 1;
    }
    fclose(in);

    FILE *out = fopen(output, "w");
    if (out == NULL) {
        fprintf(stderr, "%s: error: unable to create output file '%s': %s\n",
                TOOL, output, strerror(errno));
        return 1;
    }
    fputs("#!/bin/sh\n", out);
    fputs("echo 'Hello, World!'\n", out);
    fclose(out);
    chmod(output, 0755);

    printf("%s: compilation successful -> %s\n", TOOL, output);
    return 0;
}


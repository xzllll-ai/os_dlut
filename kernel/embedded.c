#include "console.h"
#include "embedded.h"
#include "fs.h"
#include "types.h"

extern const u8 _binary_build_user_hello_ext_elf_start[];
extern const u8 _binary_build_user_hello_ext_elf_end[];

void embedded_files_init(void) {
    size_t size = (size_t)(_binary_build_user_hello_ext_elf_end - _binary_build_user_hello_ext_elf_start);
    if (fs_write_data("/bin/hello_ext.elf", _binary_build_user_hello_ext_elf_start, size) == 0) {
        printf("[init] embedded /bin/hello_ext.elf size=%u\n", size);
    }
}

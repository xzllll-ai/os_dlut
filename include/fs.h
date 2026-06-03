#ifndef FS_H
#define FS_H

#include "types.h"

#define FS_MAX_NODES 128
#define FS_MAX_FILE_SIZE (64 * 1024)
#define FS_MAX_FD 32

#define O_RDONLY 0x1
#define O_WRONLY 0x2
#define O_RDWR   0x3
#define O_CREAT  0x100
#define O_TRUNC  0x200

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void fs_init(void);
int fs_mkdir(const char *path);
int fs_create(const char *path);
int fs_unlink(const char *path);
int fs_open(const char *path, int flags);
int fs_close(int fd);
long fs_read(int fd, void *buf, size_t n);
long fs_write(int fd, const void *buf, size_t n);
long fs_seek(int fd, long off, int whence);
int fs_list(const char *path);
int fs_cat(const char *path);
int fs_write_file(const char *path, const char *data);
const char *fs_read_file(const char *path, size_t *size);

#endif

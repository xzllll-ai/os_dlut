#include "console.h"
#include "fs.h"
#include "string.h"

typedef enum {
    FS_UNUSED,
    FS_FILE,
    FS_DIR,
} node_type_t;

struct fs_node {
    node_type_t type;
    char name[32];
    int parent;
    u8 data[FS_MAX_FILE_SIZE];
    size_t size;
};

struct file_desc {
    bool used;
    int node;
    size_t offset;
    int flags;
};

static struct fs_node nodes[FS_MAX_NODES];
static struct file_desc fds[FS_MAX_FD];

static const char *absolute_path(const char *path, char *buf, size_t len) {
    if (!path || !path[0]) {
        return "/";
    }
    if (path[0] == '/') {
        return path;
    }
    if (len < 2) {
        return path;
    }
    buf[0] = '/';
    strncpy(buf + 1, path, len - 2);
    buf[len - 1] = 0;
    return buf;
}

static int alloc_node(node_type_t type, const char *name, int parent) {
    for (int i = 1; i < FS_MAX_NODES; i++) {
        if (nodes[i].type == FS_UNUSED) {
            nodes[i].type = type;
            strncpy(nodes[i].name, name, sizeof(nodes[i].name) - 1);
            nodes[i].name[sizeof(nodes[i].name) - 1] = 0;
            nodes[i].parent = parent;
            nodes[i].size = 0;
            return i;
        }
    }
    return -1;
}

static int child_find(int parent, const char *name) {
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (nodes[i].type != FS_UNUSED && nodes[i].parent == parent && strcmp(nodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int path_walk(const char *path) {
    char abs[128];
    path = absolute_path(path, abs, sizeof(abs));
    if (!path || path[0] != '/') {
        return -1;
    }
    int cur = 0;
    const char *p = path + 1;
    char part[32];
    while (*p) {
        size_t n = 0;
        while (*p == '/') {
            p++;
        }
        while (*p && *p != '/' && n + 1 < sizeof(part)) {
            part[n++] = *p++;
        }
        part[n] = 0;
        if (n == 0) {
            break;
        }
        cur = child_find(cur, part);
        if (cur < 0) {
            return -1;
        }
    }
    return cur;
}

static int path_parent(const char *path, char *leaf, size_t leaf_len) {
    char abs[128];
    path = absolute_path(path, abs, sizeof(abs));
    if (!path || path[0] != '/') {
        return -1;
    }
    const char *last = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/') {
            last = p;
        }
    }
    if (!last[1]) {
        return -1;
    }
    strncpy(leaf, last + 1, leaf_len - 1);
    leaf[leaf_len - 1] = 0;
    if (last == path) {
        return 0;
    }
    char parent_path[128];
    size_t n = (size_t)(last - path);
    if (n >= sizeof(parent_path)) {
        return -1;
    }
    memcpy(parent_path, path, n);
    parent_path[n] = 0;
    return path_walk(parent_path);
}

void fs_init(void) {
    memset(nodes, 0, sizeof(nodes));
    memset(fds, 0, sizeof(fds));
    nodes[0].type = FS_DIR;
    strcpy(nodes[0].name, "/");
    nodes[0].parent = 0;
    fs_mkdir("/bin");
    fs_mkdir("/etc");
    fs_mkdir("/home");
    fs_mkdir("/home/user");
    fs_write_file("/etc/motd", "DLUT RISC-V teaching OS\n");
    fs_write_file("/home/user/readme.txt", "RAMFS supports create, delete, open, close, read, write and seek.\n");
}

int fs_mkdir(const char *path) {
    char leaf[32];
    int parent = path_parent(path, leaf, sizeof(leaf));
    if (parent < 0 || nodes[parent].type != FS_DIR || child_find(parent, leaf) >= 0) {
        return -1;
    }
    return alloc_node(FS_DIR, leaf, parent) < 0 ? -1 : 0;
}

int fs_create(const char *path) {
    char leaf[32];
    int parent = path_parent(path, leaf, sizeof(leaf));
    if (parent < 0 || nodes[parent].type != FS_DIR) {
        return -1;
    }
    int old = child_find(parent, leaf);
    if (old >= 0) {
        return nodes[old].type == FS_FILE ? old : -1;
    }
    return alloc_node(FS_FILE, leaf, parent);
}

int fs_unlink(const char *path) {
    int id = path_walk(path);
    if (id <= 0 || nodes[id].type == FS_UNUSED) {
        return -1;
    }
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (nodes[i].type != FS_UNUSED && nodes[i].parent == id) {
            return -1;
        }
    }
    nodes[id].type = FS_UNUSED;
    return 0;
}

int fs_open(const char *path, int flags) {
    int node = path_walk(path);
    if (node < 0 && (flags & O_CREAT)) {
        node = fs_create(path);
    }
    if (node < 0 || nodes[node].type != FS_FILE) {
        return -1;
    }
    if (flags & O_TRUNC) {
        nodes[node].size = 0;
    }
    for (int i = 0; i < FS_MAX_FD; i++) {
        if (!fds[i].used) {
            fds[i].used = true;
            fds[i].node = node;
            fds[i].offset = 0;
            fds[i].flags = flags;
            return i;
        }
    }
    return -1;
}

int fs_close(int fd) {
    if (fd < 0 || fd >= FS_MAX_FD || !fds[fd].used) {
        return -1;
    }
    fds[fd].used = false;
    return 0;
}

long fs_read(int fd, void *buf, size_t n) {
    if (fd < 0 || fd >= FS_MAX_FD || !fds[fd].used) {
        return -1;
    }
    struct fs_node *node = &nodes[fds[fd].node];
    if (fds[fd].offset >= node->size) {
        return 0;
    }
    size_t left = node->size - fds[fd].offset;
    if (n > left) {
        n = left;
    }
    memcpy(buf, node->data + fds[fd].offset, n);
    fds[fd].offset += n;
    return (long)n;
}

long fs_write(int fd, const void *buf, size_t n) {
    if (fd < 0 || fd >= FS_MAX_FD || !fds[fd].used) {
        return -1;
    }
    struct fs_node *node = &nodes[fds[fd].node];
    if (fds[fd].offset + n > FS_MAX_FILE_SIZE) {
        n = FS_MAX_FILE_SIZE - fds[fd].offset;
    }
    memcpy(node->data + fds[fd].offset, buf, n);
    fds[fd].offset += n;
    if (fds[fd].offset > node->size) {
        node->size = fds[fd].offset;
    }
    return (long)n;
}

long fs_seek(int fd, long off, int whence) {
    if (fd < 0 || fd >= FS_MAX_FD || !fds[fd].used) {
        return -1;
    }
    struct fs_node *node = &nodes[fds[fd].node];
    long base = whence == SEEK_SET ? 0 : (whence == SEEK_CUR ? (long)fds[fd].offset : (long)node->size);
    long next = base + off;
    if (next < 0 || next > FS_MAX_FILE_SIZE) {
        return -1;
    }
    fds[fd].offset = (size_t)next;
    return next;
}

int fs_list(const char *path) {
    int dir = path_walk(path && path[0] ? path : "/");
    if (dir < 0 || nodes[dir].type != FS_DIR) {
        return -1;
    }
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (nodes[i].type != FS_UNUSED && nodes[i].parent == dir && i != dir) {
            printf("%c %s %u\n", nodes[i].type == FS_DIR ? 'd' : '-', nodes[i].name, nodes[i].size);
        }
    }
    return 0;
}

int fs_cat(const char *path) {
    size_t size;
    const char *data = fs_read_file(path, &size);
    if (!data) {
        return -1;
    }
    for (size_t i = 0; i < size; i++) {
        putchar(data[i]);
    }
    if (size == 0 || data[size - 1] != '\n') {
        putchar('\n');
    }
    return 0;
}

int fs_write_file(const char *path, const char *data) {
    int fd = fs_open(path, O_CREAT | O_TRUNC | O_RDWR);
    if (fd < 0) {
        return -1;
    }
    fs_write(fd, data, strlen(data));
    fs_close(fd);
    return 0;
}

const char *fs_read_file(const char *path, size_t *size) {
    int id = path_walk(path);
    if (id < 0 || nodes[id].type != FS_FILE) {
        return NULL;
    }
    if (size) {
        *size = nodes[id].size;
    }
    return (const char *)nodes[id].data;
}

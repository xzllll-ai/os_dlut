#include "mm.h"
#include "string.h"

struct block {
    size_t size;
    bool free;
    struct block *next;
};

static struct block *heap;

static void grow_heap(void) {
    void *page = page_alloc();
    if (!page) {
        return;
    }
    struct block *b = page;
    b->size = PAGE_SIZE - sizeof(*b);
    b->free = true;
    b->next = heap;
    heap = b;
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size = ALIGN_UP(size, 16);
    for (;;) {
        for (struct block *b = heap; b; b = b->next) {
            if (b->free && b->size >= size) {
                b->free = false;
                return (void *)(b + 1);
            }
        }
        grow_heap();
        if (!heap) {
            return NULL;
        }
    }
}

void kfree(void *ptr) {
    if (!ptr) {
        return;
    }
    struct block *b = ((struct block *)ptr) - 1;
    b->free = true;
}

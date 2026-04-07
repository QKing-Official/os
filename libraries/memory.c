#include "memory.h"

// My basic memory driver
// Soon this will be replaced by a vfs

static uint8_t *heap_base = 0;
static size_t heap_capacity = 0;
static size_t heap_used = 0;

void memory_init(uint64_t start, uint64_t size) {
    heap_base = (uint8_t *)start;
    heap_capacity = size;
    heap_used = 0;
}

void *kmalloc(size_t size) {
    if (!heap_base || heap_used + size > heap_capacity)
        return 0; // out of memory
    void *ptr = heap_base + heap_used;
    heap_used += size;
    return ptr;
}

void kfree(void *ptr) {
    (void)ptr;
}

size_t memory_free_space(void) {
    if (!heap_base) return 0;
    return heap_capacity - heap_used;
}
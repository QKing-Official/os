// userspace/memtest/memtest.c
#include "../../libraries/memory.h"
#include "../../libraries/timer.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/libc/include/libc.h"
#include "../userspace.h"

static void format_size(uint64_t bytes, char *out) {
    const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    uint64_t val = bytes;

    while (val >= 1024 && idx < 4) {
        val /= 1024;
        idx++;
    }

    char buf[32];
    int i = 0;
    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val) {
            buf[i++] = '0' + (val % 10);
            val /= 10;
        }
    }
    // Uno Reverse
    int j = 0;
    while (i > 0) buf[j++] = buf[--i];
    buf[j] = '\0';

    int pos = 0;
    for (int k = 0; buf[k]; k++) out[pos++] = buf[k];
    int l = 0;
    while (suffixes[idx][l]) out[pos++] = suffixes[idx][l++];
    out[pos] = '\0';
}

void memtest_main(void) {
    char size_buf[32];

    printf("Starting memory test...\n");

    void *a = kmalloc(1024);
    void *b = kmalloc(2048);
    void *c = kmalloc(4096);

    printf("Allocated blocks:\n");
    printf("a = %p\n", a);
    printf("b = %p\n", b);
    printf("c = %p\n", c);

    format_size(memory_free_space(), size_buf);
    printf("Free memory: %s\n", size_buf);

    kfree(b);
    printf("Freed middle block (b)\n");
    format_size(memory_free_space(), size_buf);
    printf("Free memory: %s\n", size_buf);

    void *d = kmalloc(1024);
    printf("Allocated new block after free: %p\n", d);
    format_size(memory_free_space(), size_buf);
    printf("Free memory: %s\n", size_buf);

    kfree(a);
    kfree(c);
    kfree(d);
    printf("Freed all blocks\n");
    format_size(memory_free_space(), size_buf);
    printf("Free memory: %s\n", size_buf);

    // Wait for exit
    printf("\nPress Q to exit memory test\n");
    keyboard_init();
    while (1) {
        if (keyboard_has_key()) {
            char c = keyboard_read();
            if (c == 'q' || c == 'Q')
                break;
        }
        timer_delay_ms(50);
    }
}

int memtest_test(void) { return 1; }

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program memtest_prog = {
    .name = "memtest",
    .main = memtest_main,
    .test = memtest_test
};
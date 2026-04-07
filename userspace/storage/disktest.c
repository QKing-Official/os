// userspace/memtest/disktest.c
#include "../../libraries/libc/include/libc.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/timer.h"
#include "../userspace.h"
#include "../storage/sfs.h"

#define TEST_FILENAME "testfile.txt"
#define TEST_DATA "Hello Lockinos! This is a test of SFS persistence.\n"

void disktest_main(void) {
    char read_buf[256];

    printf("Disk Test\n");

    if (sfs_init() != 0) {
        printf("SFS initialization failed!\n");
        return;
    }

    printf("\nWriting file: %s\n", TEST_FILENAME);
    int bytes_written = sfs_write(TEST_FILENAME, TEST_DATA, strlen(TEST_DATA));
    if (bytes_written < 0) {
        printf("Error writing file!\n");
    } else {
        printf("Wrote %d bytes\n", bytes_written);
    }

    memset(read_buf, 0, sizeof(read_buf));
    int bytes_read = sfs_read(TEST_FILENAME, read_buf, sizeof(read_buf));
    if (bytes_read < 0) {
        printf("Error reading file immediately after write!\n");
    } else {
        printf("Read %d bytes: \"%s\"\n", bytes_read, read_buf);
    }

    printf("\nSimulating reboot (re-init SFS)...\n");
    if (sfs_init() != 0) {
        printf("SFS re-initialization failed!\n");
        return;
    }

    memset(read_buf, 0, sizeof(read_buf));
    bytes_read = sfs_read(TEST_FILENAME, read_buf, sizeof(read_buf));
    if (bytes_read < 0) {
        printf("Error reading file after re-init!\n");
    } else {
        printf("Read %d bytes after re-init: \"%s\"\n", bytes_read, read_buf);
    }

    printf("\nDisk test complete. Press Q to exit.\n");

    keyboard_init();
    while (1) {
        if (keyboard_has_key()) {
            char c = keyboard_read();
            if (c == 'q' || c == 'Q') break;
        }
        timer_delay_ms(50);
    }
}

int disktest_test(void) { return 1; }

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program disktest_prog = {
    .name = "disktest",
    .main = disktest_main,
    .test = disktest_test
};
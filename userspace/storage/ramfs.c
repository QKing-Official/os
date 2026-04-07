#include "ramfs.h"
#include <string.h>
#include <stdint.h>

#define MAX_FILES 16
#define MAX_NAME 32
#define MAX_SIZE 512

typedef struct {
    char name[MAX_NAME];
    uint32_t size;
    uint8_t data[MAX_SIZE];
} ram_inode_t;

static ram_inode_t files[MAX_FILES];

int ramfs_init() {
    memset(files, 0, sizeof(files));
    return 0;
}

int ramfs_write(const char* name, const void* buf, uint32_t size) {
    ram_inode_t* inode = NULL;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] == '\0') { inode = &files[i]; break; }
        if (strncmp(files[i].name, name, MAX_NAME) == 0) { inode = &files[i]; break; }
    }
    if (!inode) return -1;
    if (size > MAX_SIZE) size = MAX_SIZE;

    strncpy(inode->name, name, MAX_NAME-1);
    memcpy(inode->data, buf, size);
    inode->size = size;
    return size;
}

int ramfs_read(const char* name, void* buf, uint32_t max_size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (strncmp(files[i].name, name, MAX_NAME) == 0) {
            uint32_t sz = (files[i].size > max_size) ? max_size : files[i].size;
            memcpy(buf, files[i].data, sz);
            return sz;
        }
    }
    return 0;
}
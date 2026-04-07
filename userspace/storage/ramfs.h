#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>

int ramfs_init();
int ramfs_write(const char* name, const void* buf, uint32_t size);
int ramfs_read(const char* name, void* buf, uint32_t max_size);

#endif
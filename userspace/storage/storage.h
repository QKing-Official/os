#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

int storage_init();
int storage_read_block(uint32_t lba, void* buf);
int storage_write_block(uint32_t lba, const void* buf);

#endif
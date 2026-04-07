#include "sfs.h"
#include "storage.h"
#include <string.h>
#include <stdint.h>

sfs_superblock_t superblock;

int sfs_init(void) {
    if (storage_read_block(0, &superblock) != 0) return -1;
    if (superblock.magic != SFS_MAGIC) {
        superblock.magic = SFS_MAGIC;
        superblock.total_blocks = 128;
        superblock.inode_table_start = 1;
        superblock.data_start = 10;
        storage_write_block(0, &superblock);
    }
    return 0;
}

int sfs_write(const char* name, const void* buf, uint32_t size) {
    uint8_t block[512];
    memset(block, 0, 512);
    strncpy((char*)block, name, 31);
    memcpy(block + 32, buf, size > 480 ? 480 : size);
    return storage_write_block(superblock.data_start, block);
}

int sfs_read(const char* name, void* buf, uint32_t max_size) {
    uint8_t block[512];
    if (storage_read_block(superblock.data_start, block) != 0) return -1;
    if (strncmp((char*)block, name, 31) != 0) return -1;
    uint32_t size = max_size > 480 ? 480 : max_size;
    memcpy(buf, block + 32, size);
    return size;
}
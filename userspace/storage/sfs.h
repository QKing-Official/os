#ifndef SFS_H
#define SFS_H

#include <stdint.h>

#define SFS_MAGIC 0x53465321U
#define SFS_BLOCK_SIZE 512

typedef struct sfs_inode {
    char name[32];
    uint32_t size;
    uint32_t start_block;
} sfs_inode_t;

typedef struct sfs_superblock {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t inode_table_start;
    uint32_t data_start;
} sfs_superblock_t;

int sfs_init();
int sfs_read(const char* name, void* buf, uint32_t max_size);
int sfs_write(const char* name, const void* buf, uint32_t size);

#endif
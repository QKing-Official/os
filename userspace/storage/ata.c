#include "ata.h"
#include <stdint.h>

#define SECTOR_SIZE 512

static uint8_t disk_image[128 * SECTOR_SIZE];

void ata_init() {
    // Nothing needed for QEMU IDE
}

int ata_read_sector(uint32_t lba, void* buf) {
    if (lba >= 128) return -1;
    for (int i = 0; i < SECTOR_SIZE; i++)
        ((uint8_t*)buf)[i] = disk_image[lba * SECTOR_SIZE + i];
    return 0;
}

int ata_write_sector(uint32_t lba, const void* buf) {
    if (lba >= 128) return -1;
    for (int i = 0; i < SECTOR_SIZE; i++)
        disk_image[lba * SECTOR_SIZE + i] = ((uint8_t*)buf)[i];
    return 0;
}
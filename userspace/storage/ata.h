#ifndef ATA_H
#define ATA_H

#include <stdint.h>

void ata_init(void);
int ata_read_sector(uint32_t lba, void* buf);
int ata_write_sector(uint32_t lba, const void* buf);

#endif
#include "storage.h"
#include "ata.h"

int storage_init() {
    ata_init();
    return 0;
}

int storage_read_block(uint32_t lba, void* buf) {
    return ata_read_sector(lba, buf);
}

int storage_write_block(uint32_t lba, const void* buf) {
    return ata_write_sector(lba, buf);
}
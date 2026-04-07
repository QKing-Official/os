#include "vfs.h"
#include "sfs.h"
#include "ramfs.h"

int vfs_init() {
    sfs_init();
    ramfs_init();
    return 0;
}
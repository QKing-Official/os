
#include "../include/libc.h"

/*
 * TEMPORARILY DISABLED memcpy
 *
 * You already had a memcpy inside another module (e.g. draw.c),
 * which caused a multiple-definition linker error.
 *
 * Uncomment this once the old memcpy is removed.
 */

/*
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    for (size_t i = 0; i < n; i++)
        d[i] = s[i];

    return dst;
}
*/

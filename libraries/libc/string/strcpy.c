
#include "../include/libc.h"

char *strcpy(char *dst, const char *src)
{
    char *r = dst;
    while ((*dst++ = *src++));
    return r;
}

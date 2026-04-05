
#include "../include/libc.h"
#include <stdarg.h>

static void print_int(int v)
{
    char buf[16];
    int i = 0;

    if (v == 0) {
        putchar('0');
        return;
    }

    if (v < 0) {
        putchar('-');
        v = -v;
    }

    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    while (i--)
        putchar(buf[i]);
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 's') {
                char *s = va_arg(args, char *);
                while (*s) putchar(*s++);
            } else if (*fmt == 'd') {
                print_int(va_arg(args, int));
            } else if (*fmt == 'c') {
                putchar(va_arg(args, int));
            } else {
                putchar('%');
                putchar(*fmt);
            }
        } else {
            putchar(*fmt);
        }
        fmt++;
    }

    va_end(args);
    return 0;
}

#include "../include/libc.h"
// libraries/libc/stdio/printf.c
#include <stdarg.h>

/* Simple freestanding printf for OS */
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int chars_printed = 0;

    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            putchar(fmt[i]);
            chars_printed++;
            continue;
        }

        i++;
        switch (fmt[i]) {
            case 's': {
                const char *s = va_arg(args, const char *);
                while (*s) {
                    putchar(*s++);
                    chars_printed++;
                }
                break;
            }

            case 'd': {
                int val = va_arg(args, int);
                char buf[32];
                int j = 0, neg = 0;

                if (val == 0) {
                    putchar('0');
                    chars_printed++;
                    break;
                }

                if (val < 0) {
                    neg = 1;
                    val = -val;
                }

                while (val) {
                    buf[j++] = '0' + (val % 10);
                    val /= 10;
                }

                if (neg) {
                    putchar('-');
                    chars_printed++;
                }

                while (j--) {
                    putchar(buf[j]);
                    chars_printed++;
                }
                break;
            }

            case 'x': {
                unsigned long val = va_arg(args, unsigned long);
                char hex[32];
                int j = 0;

                if (val == 0) {
                    putchar('0');
                    chars_printed++;
                    break;
                }

                while (val) {
                    uint8_t d = val & 0xF;
                    hex[j++] = d < 10 ? '0' + d : 'a' + (d - 10);
                    val >>= 4;
                }

                while (j--) {
                    putchar(hex[j]);
                    chars_printed++;
                }
                break;
            }

            case 'p': {
                void *ptr = va_arg(args, void *);
                unsigned long val = (unsigned long)ptr;
                char hex[32];
                int j = 0;

                putchar('0');
                putchar('x');
                chars_printed += 2;

                if (val == 0) {
                    putchar('0');
                    chars_printed++;
                    break;
                }

                while (val) {
                    uint8_t d = val & 0xF;
                    hex[j++] = d < 10 ? '0' + d : 'a' + (d - 10);
                    val >>= 4;
                }

                while (j--) {
                    putchar(hex[j]);
                    chars_printed++;
                }
                break;
            }

            case '%':
                putchar('%');
                chars_printed++;
                break;

            default:
                putchar('?'); // Unknown specifier
                chars_printed++;
                break;
        }
    }

    va_end(args);
    return chars_printed;
}

// Printf is just needed
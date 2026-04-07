#include "../include/libc.h"
#include <stdarg.h>
#include <stddef.h>

static size_t write_char(char *buf, size_t pos, size_t cap, char c) {
    if (pos < cap - 1) {
        buf[pos] = c;
    }
    return pos + 1;
}

static size_t write_str(char *buf, size_t pos, size_t cap, const char *s, size_t len) {
    for (size_t i = 0; i < len && s[i]; i++) {
        pos = write_char(buf, pos, cap, s[i]);
    }
    return pos;
}

static size_t format_unsigned(char *out, unsigned long long val, int width, int zero_pad) {
    char temp[32];
    int idx = 0;

    do {
        temp[idx++] = "0123456789"[val % 10];
        val /= 10;
    } while (val);

    int len = idx;
    int pad = (width > len) ? (width - len) : 0;

    size_t written = 0;
    for (int i = 0; i < pad; i++) {
        out[written++] = zero_pad ? '0' : ' ';
    }
    for (int i = len - 1; i >= 0; i--) {
        out[written++] = temp[i];
    }
    return written;
}

static size_t format_signed(char *out, long long val, int width, int zero_pad) {
    if (val < 0) {
        out[0] = '-';
        size_t len = format_unsigned(out + 1, -val, width, zero_pad);
        return 1 + len;
    } else {
        return format_unsigned(out, val, width, zero_pad);
    }
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    size_t pos = 0;
    size_t cap = n;
    for (size_t f = 0; fmt[f] && pos < cap - 1; f++) {
        if (fmt[f] != '%') {
            pos = write_char(buf, pos, cap, fmt[f]);
            continue;
        }

        f++;
        if (!fmt[f]) break;

        int zero_pad = 0;
        int width = 0;

        /* Parse flag '0' */
        if (fmt[f] == '0') {
            zero_pad = 1;
            f++;
        }

        /* Parse width (digits) */
        while (fmt[f] >= '0' && fmt[f] <= '9') {
            width = width * 10 + (fmt[f] - '0');
            f++;
        }

        char conv = fmt[f];
        char temp_buf[64];

        switch (conv) {
            case 's': {
                char *s = va_arg(args, char*);
                if (s == NULL) s = "(null)";
                /* For simplicity, width is ignored for %s */
                for (size_t i = 0; s[i] && pos < cap - 1; i++) {
                    pos = write_char(buf, pos, cap, s[i]);
                }
                break;
            }
            case 'd': {
                int val = va_arg(args, int);
                size_t len = format_signed(temp_buf, val, width, zero_pad);
                pos = write_str(buf, pos, cap, temp_buf, len);
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                size_t len = format_unsigned(temp_buf, val, width, zero_pad);
                pos = write_str(buf, pos, cap, temp_buf, len);
                break;
            }
            case 'p': {
                void *ptr = va_arg(args, void*);
                unsigned long long val = (unsigned long long)ptr;
                temp_buf[0] = '0';
                temp_buf[1] = 'x';
                size_t len = format_unsigned(temp_buf + 2, val, 0, 0);
                len += 2;
                pos = write_str(buf, pos, cap, temp_buf, len);
                break;
            }
            default:
                pos = write_char(buf, pos, cap, conv);
                break;
        }
    }

    if (n > 0) {
        buf[pos < n ? pos : n - 1] = '\0';
    }
    va_end(args);
    return (int)pos;
}
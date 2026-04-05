
#include "../../draw.h"
#include "../include/libc.h"

static int cursor_x = 10;
static int cursor_y = 80;

int putchar(int c)
{
    char s[2];
    s[0] = (char)c;
    s[1] = 0;

    if (c == '\n') {
        cursor_x = 10;
        cursor_y += 16;
        return c;
    }

    draw_string(cursor_x, cursor_y, s, 1);
    cursor_x += 8;

    return c;
}

#include "draw.h"
#include "font.h"

static struct limine_framebuffer *_fb;

void draw_init(struct limine_framebuffer *framebuffer) {
    _fb = framebuffer;
}

uint32_t screen_width(void)  { return _fb->width; }
uint32_t screen_height(void) { return _fb->height; }

void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= _fb->width || y >= _fb->height) return;
    volatile uint32_t *ptr =
        (volatile uint32_t *)((uint8_t *)_fb->address + y * _fb->pitch + x * (_fb->bpp / 8));
    *ptr = color;
}

void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t r = y; r < y + h; r++)
        for (uint32_t c = x; c < x + w; c++)
            put_pixel(c, r, color);
}

void draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                       uint32_t thickness, uint32_t color) {
    fill_rect(x, y, w, thickness, color);
    fill_rect(x, y + h - thickness, w, thickness, color);
    fill_rect(x, y, thickness, h, color);
    fill_rect(x + w - thickness, y, thickness, h, color);
}

void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    int dx = (int)x1 - (int)x0, dy = (int)y1 - (int)y0;
    int steps = dx < 0 ? -dx : dx;
    if ((dy < 0 ? -dy : dy) > steps) steps = (dy < 0 ? -dy : dy);
    if (steps == 0) { put_pixel(x0, y0, color); return; }
    for (int i = 0; i <= steps; i++) {
        put_pixel(x0 + i * dx / steps, y0 + i * dy / steps, color);
    }
}

void draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    int x = 0, y = (int)r, d = 1 - (int)r;
    while (x <= y) {
        put_pixel(cx+x, cy+y, color); put_pixel(cx-x, cy+y, color);
        put_pixel(cx+x, cy-y, color); put_pixel(cx-x, cy-y, color);
        put_pixel(cx+y, cy+x, color); put_pixel(cx-y, cy+x, color);
        put_pixel(cx+y, cy-x, color); put_pixel(cx-y, cy-x, color);
        if (d < 0) d += 2*x+3; else { d += 2*(x-y)+5; y--; }
        x++;
    }
}

void fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    for (int y = -(int)r; y <= (int)r; y++)
        for (int x = -(int)r; x <= (int)r; x++)
            if (x*x + y*y <= (int)(r*r))
                put_pixel(cx+x, cy+y, color);
}

static uint32_t _fg = 0xFFFFFFFF;
static uint32_t _bg = 0xFF0D1117;

void set_fg(uint32_t color) { _fg = color; }
void set_bg(uint32_t color) { _bg = color; }

void draw_char(uint32_t x, uint32_t y, char c, uint32_t scale) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = font8x8[(uint8_t)c - 32];
    for (uint32_t row = 0; row < 8; row++) {
        for (uint32_t col = 0; col < 8; col++) {
            uint32_t color = (glyph[row] & (1 << col)) ? _fg : _bg;
            fill_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

void draw_string(uint32_t x, uint32_t y, const char *str, uint32_t scale) {
    uint32_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += 8 * scale;
        } else {
            draw_char(cx, y, *str, scale);
            cx += 8 * scale;
        }
        str++;
    }
}

// for userspace ui
uint32_t get_fg(void) { return _fg; }
uint32_t get_bg(void) { return _bg; }
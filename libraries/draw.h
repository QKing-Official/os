#pragma once
#include <stdint.h>
#include "limine.h"

void draw_init(struct limine_framebuffer *framebuffer);

// Shapes for fun!
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                       uint32_t thickness, uint32_t color);
void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
void draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);
void fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);

// Text
void set_fg(uint32_t color);
void set_bg(uint32_t color);
void draw_char(uint32_t x, uint32_t y, char c, uint32_t scale);
void draw_string(uint32_t x, uint32_t y, const char *str, uint32_t scale);

// Screen info cuz the fucking thing has to know it and I just cant continue without it....
uint32_t screen_width(void);
uint32_t screen_height(void);

// Variables for userspace ui 
void set_fg(uint32_t color);
void set_bg(uint32_t color);
uint32_t get_fg(void);
uint32_t get_bg(void);
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "limine.h"

// Drawing API
void draw_init(struct limine_framebuffer *framebuffer);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                       uint32_t thickness, uint32_t color);
void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
void draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);
void fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);
void set_fg(uint32_t color);
void set_bg(uint32_t color);
void draw_char(uint32_t x, uint32_t y, char c, uint32_t scale);
void draw_string(uint32_t x, uint32_t y, const char *str, uint32_t scale);
uint32_t screen_width(void);
uint32_t screen_height(void);
uint32_t get_fg(void);
uint32_t get_bg(void);

// GPU hardware info
typedef enum {
    GPU_VENDOR_UNKNOWN,
    GPU_VENDOR_INTEL,
    GPU_VENDOR_AMD,
    GPU_VENDOR_NVIDIA,
    GPU_VENDOR_VMWARE,
    GPU_VENDOR_VIRTIO,
} gpu_vendor_t;

typedef struct {
    gpu_vendor_t vendor;
    uint16_t device_id;
    uint16_t vendor_id;
    uint64_t vram_size;
    uint32_t revision;
    uint8_t  bus, slot, func;
    char     model_str[64];
} gpu_info_t;

int gpu_is_hardware_accelerated(void);
const gpu_info_t* gpu_get_info(void);
uint64_t gpu_get_vram_size(void);
uint64_t gpu_get_free_vram(void);
uint64_t gpu_get_used_vram(void);
void gpu_memcpy_to_vram(uint64_t vram_offset, const void *src, size_t size);
void gpu_memcpy_from_vram(void *dst, uint64_t vram_offset, size_t size);
#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

// GPU vendor enum
typedef enum {
    GPU_VENDOR_UNKNOWN,
    GPU_VENDOR_INTEL,
    GPU_VENDOR_AMD,
    GPU_VENDOR_NVIDIA,
    GPU_VENDOR_VMWARE,
    GPU_VENDOR_VIRTIO
} gpu_vendor_t;

// GPU information structure
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint8_t revision;
    gpu_vendor_t vendor;
    uint64_t vram_size;
    char model_str[64];
} gpu_info_t;

// Driver function pointer structure
struct gpu_driver {
    void (*init)(struct limine_framebuffer *fb);
    void (*put_pixel)(uint32_t x, uint32_t y, uint32_t color);
    void (*fill_rect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void (*draw_rect_outline)(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                              uint32_t thickness, uint32_t color);
    void (*draw_line)(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
    void (*draw_circle)(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);
    void (*fill_circle)(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color);
    void (*set_fg)(uint32_t color);
    void (*set_bg)(uint32_t color);
    void (*draw_char)(uint32_t x, uint32_t y, char c, uint32_t scale);
    void (*draw_string)(uint32_t x, uint32_t y, const char *str, uint32_t scale);
    uint32_t (*get_width)(void);
    uint32_t (*get_height)(void);
    uint32_t (*get_fg)(void);
    uint32_t (*get_bg)(void);
};

// Public API
void draw_init(struct limine_framebuffer *framebuffer);
int gpu_is_hardware_accelerated(void);
const gpu_info_t* gpu_get_info(void);
uint64_t gpu_get_vram_size(void);
uint64_t gpu_get_free_vram(void);
uint64_t gpu_get_used_vram(void);

// VRAM allocation (simple tracking)
uint64_t gpu_alloc_vram(uint64_t size);
void gpu_free_vram(uint64_t offset);

void gpu_memcpy_to_vram(uint64_t vram_offset, const void *src, size_t size);
void gpu_memcpy_from_vram(void *dst, uint64_t vram_offset, size_t size);

// Legacy drawing functions
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

#endif // DRAW_H
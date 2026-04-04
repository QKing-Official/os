#include "draw.h"
#include "font.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// IO
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// PCI Conf
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31) | (bus << 16) | ((slot & 0x1F) << 11) | ((func & 0x07) << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static uint16_t pci_read_vendor_id(uint8_t bus, uint8_t slot, uint8_t func) {
    return pci_read_config(bus, slot, func, 0) & 0xFFFF;
}

static uint16_t pci_read_device_id(uint8_t bus, uint8_t slot, uint8_t func) {
    return (pci_read_config(bus, slot, func, 0) >> 16) & 0xFFFF;
}

static uint8_t pci_read_revision_id(uint8_t bus, uint8_t slot, uint8_t func) {
    return pci_read_config(bus, slot, func, 8) & 0xFF;
}

static uint8_t pci_read_class_code(uint8_t bus, uint8_t slot, uint8_t func, uint8_t *subclass) {
    uint32_t val = pci_read_config(bus, slot, func, 8);
    uint8_t base = (val >> 24) & 0xFF;
    if (subclass) *subclass = (val >> 16) & 0xFF;
    return base;
}

// Debug
#define DEBUG_PCI
#ifdef DEBUG_PCI
static void debug_print(const char *str) {
    static int line = 0;
    set_fg(0xFFFF00FF);
    draw_string(0, line * 10, str, 1);
    line++;
}
#endif

// adress to vendor
static void format_model_string(char *out, size_t max_len, uint16_t vendor_id, uint16_t device_id) {
    const char *vendor_name;
    switch (vendor_id) {
        case 0x8086: vendor_name = "Intel"; break;
        case 0x1002: vendor_name = "AMD"; break;
        case 0x10DE: vendor_name = "NVIDIA"; break;
        case 0x15AD: vendor_name = "VMware"; break;
        case 0x1AF4: vendor_name = "VirtIO"; break;
        default:     vendor_name = "Unknown"; break;
    }
    // manual hex
    const char hex[] = "0123456789ABCDEF";
    char vend_hex[5], dev_hex[5];
    for (int i = 3; i >= 0; i--) {
        vend_hex[i] = hex[(vendor_id >> (4*(3-i))) & 0xF];
        dev_hex[i]  = hex[(device_id  >> (4*(3-i))) & 0xF];
    }
    vend_hex[4] = dev_hex[4] = '\0';
    int pos = 0;
    while (pos < max_len-1 && vendor_name[pos]) {
        out[pos] = vendor_name[pos];
        pos++;
    }
    const char *mid = " GPU (0x";
    for (int i = 0; mid[i] && pos < max_len-1; i++) out[pos++] = mid[i];
    for (int i = 0; i < 4 && pos < max_len-1; i++) out[pos++] = vend_hex[i];
    if (pos < max_len-1) out[pos++] = ':';
    for (int i = 0; i < 4 && pos < max_len-1; i++) out[pos++] = dev_hex[i];
    if (pos < max_len-1) out[pos++] = ')';
    out[pos] = '\0';
}

// Scans for gpu
static gpu_info_t gpu_info = {0};
static int hardware_accelerated = 0;

static void detect_gpu(void) {
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint16_t vendor = pci_read_vendor_id(bus, slot, func);
                if (vendor == 0xFFFF || vendor == 0x0000) {
                    if (func == 0) break;
                    else continue;
                }
                uint8_t subclass;
                uint8_t base_class = pci_read_class_code(bus, slot, func, &subclass);
                if (base_class == 0x03 && (subclass == 0x00 || subclass == 0x01)) { // VGA or XGA
                    gpu_info.vendor_id = vendor;
                    gpu_info.device_id = pci_read_device_id(bus, slot, func);
                    gpu_info.bus = bus;
                    gpu_info.slot = slot;
                    gpu_info.func = func;
                    gpu_info.revision = pci_read_revision_id(bus, slot, func);

                    switch (vendor) {
                        case 0x8086: gpu_info.vendor = GPU_VENDOR_INTEL; break;
                        case 0x1002: gpu_info.vendor = GPU_VENDOR_AMD;   break;
                        case 0x10DE: gpu_info.vendor = GPU_VENDOR_NVIDIA; break;
                        case 0x15AD: gpu_info.vendor = GPU_VENDOR_VMWARE; break;
                        case 0x1AF4: gpu_info.vendor = GPU_VENDOR_VIRTIO; break;
                        default:     gpu_info.vendor = GPU_VENDOR_UNKNOWN;
                    }

                    // Estimate VRAM size
                    uint32_t bar0 = pci_read_config(bus, slot, func, 0x10);
                    uint64_t vram_size = 0;
                    if ((bar0 & 0x01) == 0) { // memory BAR
                        uint32_t old = bar0;
                        outl(PCI_CONFIG_ADDRESS, (1U<<31)|(bus<<16)|((slot&0x1F)<<11)|((func&7)<<8)|0x10);
                        outl(PCI_CONFIG_DATA, 0xFFFFFFFF);
                        uint32_t size_raw = inl(PCI_CONFIG_DATA);
                        outl(PCI_CONFIG_ADDRESS, (1U<<31)|(bus<<16)|((slot&0x1F)<<11)|((func&7)<<8)|0x10);
                        outl(PCI_CONFIG_DATA, old);
                        if (size_raw) {
                            size_raw = (~size_raw) + 1;
                            vram_size = size_raw;
                        }
                    }
                    if (vram_size == 0) {
                        // fallback for the sake of safety
                        vram_size = 128ULL * 1024 * 1024;
                        if (vendor == 0x1AF4) vram_size = 256ULL * 1024 * 1024;
                    }
                    gpu_info.vram_size = vram_size;

                    format_model_string(gpu_info.model_str, sizeof(gpu_info.model_str), vendor, gpu_info.device_id);
                    hardware_accelerated = 1;
                    return;
                }
                if (func == 0 && vendor == 0xFFFF) break;
            }
        }
    }
    // 404 Not found
    hardware_accelerated = 0;
    gpu_info.vendor = GPU_VENDOR_UNKNOWN;
    gpu_info.vram_size = 0;
    const char *fallback = "Software Framebuffer";
    for (size_t i = 0; i < sizeof(gpu_info.model_str)-1 && fallback[i]; i++)
        gpu_info.model_str[i] = fallback[i];
    gpu_info.model_str[sizeof(gpu_info.model_str)-1] = '\0';
}

// Driver itself
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

static struct gpu_driver *active_driver = NULL;

// Software Framebuffer Backend Static
typedef struct {
    volatile uint8_t *address;
    uint32_t width, height, pitch, bpp;
} fb_info_t;

static fb_info_t fb;
static uint32_t fg_color = 0xFFFFFFFF;
static uint32_t bg_color = 0xFF0D1117;

static void sw_init(struct limine_framebuffer *framebuffer) {
    fb.address = (volatile uint8_t *)framebuffer->address;
    fb.width = framebuffer->width;
    fb.height = framebuffer->height;
    fb.pitch = framebuffer->pitch;
    fb.bpp = framebuffer->bpp;
}

static void sw_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb.width || y >= fb.height) return;
    volatile uint32_t *ptr = (volatile uint32_t *)(fb.address + y * fb.pitch + x * (fb.bpp / 8));
    *ptr = color;
}

static void sw_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (x >= fb.width || y >= fb.height) return;
    if (x + w > fb.width) w = fb.width - x;
    if (y + h > fb.height) h = fb.height - y;
    uint32_t bpp_bytes = fb.bpp / 8;
    volatile uint8_t *row_start = fb.address + y * fb.pitch + x * bpp_bytes;
    if (bpp_bytes == 4 && (fb.pitch % 4 == 0) && (x * 4 % 4 == 0)) {
        volatile uint32_t *row = (volatile uint32_t *)row_start;
        uint32_t words_per_row = w;
        for (uint32_t i = 0; i < h; i++) {
            for (uint32_t j = 0; j < words_per_row; j++) row[j] = color;
            row = (volatile uint32_t *)((volatile uint8_t *)row + fb.pitch);
        }
    } else {
        for (uint32_t i = 0; i < h; i++) {
            volatile uint8_t *row = row_start + i * fb.pitch;
            for (uint32_t j = 0; j < w; j++) {
                for (uint32_t b = 0; b < bpp_bytes; b++)
                    row[j * bpp_bytes + b] = (color >> (b * 8)) & 0xFF;
            }
        }
    }
}

static void sw_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                 uint32_t thickness, uint32_t color) {
    sw_fill_rect(x, y, w, thickness, color);
    sw_fill_rect(x, y + h - thickness, w, thickness, color);
    sw_fill_rect(x, y, thickness, h, color);
    sw_fill_rect(x + w - thickness, y, thickness, h, color);
}

static void sw_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    int dx = (int)x1 - (int)x0, dy = (int)y1 - (int)y0;
    int steps = dx < 0 ? -dx : dx;
    if ((dy < 0 ? -dy : dy) > steps) steps = (dy < 0 ? -dy : dy);
    if (steps == 0) { sw_put_pixel(x0, y0, color); return; }
    for (int i = 0; i <= steps; i++) {
        sw_put_pixel(x0 + i * dx / steps, y0 + i * dy / steps, color);
    }
}

static void sw_draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    int x = 0, y = (int)r, d = 1 - (int)r;
    while (x <= y) {
        sw_put_pixel(cx+x, cy+y, color); sw_put_pixel(cx-x, cy+y, color);
        sw_put_pixel(cx+x, cy-y, color); sw_put_pixel(cx-x, cy-y, color);
        sw_put_pixel(cx+y, cy+x, color); sw_put_pixel(cx-y, cy+x, color);
        sw_put_pixel(cx+y, cy-x, color); sw_put_pixel(cx-y, cy-x, color);
        if (d < 0) d += 2*x+3; else { d += 2*(x-y)+5; y--; }
        x++;
    }
}

static void sw_fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    for (int y = -(int)r; y <= (int)r; y++)
        for (int x = -(int)r; x <= (int)r; x++)
            if (x*x + y*y <= (int)(r*r))
                sw_put_pixel(cx+x, cy+y, color);
}

static void sw_set_fg(uint32_t color) { fg_color = color; }
static void sw_set_bg(uint32_t color) { bg_color = color; }

static void sw_draw_char(uint32_t x, uint32_t y, char c, uint32_t scale) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = font8x8[(uint8_t)c - 32];
    for (uint32_t row = 0; row < 8; row++) {
        for (uint32_t col = 0; col < 8; col++) {
            uint32_t color = (glyph[row] & (1 << col)) ? fg_color : bg_color;
            sw_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

static void sw_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t scale) {
    uint32_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += 8 * scale;
        } else {
            sw_draw_char(cx, y, *str, scale);
            cx += 8 * scale;
        }
        str++;
    }
}

static uint32_t sw_get_width(void)  { return fb.width; }
static uint32_t sw_get_height(void) { return fb.height; }
static uint32_t sw_get_fg(void)     { return fg_color; }
static uint32_t sw_get_bg(void)     { return bg_color; }

static struct gpu_driver sw_driver = {
    .init = sw_init,
    .put_pixel = sw_put_pixel,
    .fill_rect = sw_fill_rect,
    .draw_rect_outline = sw_draw_rect_outline,
    .draw_line = sw_draw_line,
    .draw_circle = sw_draw_circle,
    .fill_circle = sw_fill_circle,
    .set_fg = sw_set_fg,
    .set_bg = sw_set_bg,
    .draw_char = sw_draw_char,
    .draw_string = sw_draw_string,
    .get_width = sw_get_width,
    .get_height = sw_get_height,
    .get_fg = sw_get_fg,
    .get_bg = sw_get_bg,
};

// Hardware Backend Placeholder
static void hw_init(struct limine_framebuffer *framebuffer) {
    sw_init(framebuffer);
    // TODO: INIT REAL GPU
}

static void hw_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    // TODO: GPU‑accelerated rectangle fill
    sw_fill_rect(x, y, w, h, color);
}

static struct gpu_driver hw_driver = {
    .init = hw_init,
    .put_pixel = sw_put_pixel,
    .fill_rect = hw_fill_rect,
    .draw_rect_outline = sw_draw_rect_outline,
    .draw_line = sw_draw_line,
    .draw_circle = sw_draw_circle,
    .fill_circle = sw_fill_circle,
    .set_fg = sw_set_fg,
    .set_bg = sw_set_bg,
    .draw_char = sw_draw_char,
    .draw_string = sw_draw_string,
    .get_width = sw_get_width,
    .get_height = sw_get_height,
    .get_fg = sw_get_fg,
    .get_bg = sw_get_bg,
};

// API
void draw_init(struct limine_framebuffer *framebuffer) {
    detect_gpu();
    if (hardware_accelerated) {
        hw_driver.init(framebuffer);
        active_driver = &hw_driver;
    } else {
        sw_driver.init(framebuffer);
        active_driver = &sw_driver;
    }
}

int gpu_is_hardware_accelerated(void) {
    return hardware_accelerated;
}

const gpu_info_t* gpu_get_info(void) {
    return &gpu_info;
}

uint64_t gpu_get_vram_size(void) {
    return gpu_info.vram_size;
}

uint64_t gpu_get_free_vram(void) {
    return gpu_info.vram_size; // placeholder: assume all free
}

uint64_t gpu_get_used_vram(void) {
    return 0;
}

void gpu_memcpy_to_vram(uint64_t vram_offset, const void *src, size_t size) {
    (void)vram_offset; (void)src; (void)size;
    // Real driver!
}

void gpu_memcpy_from_vram(void *dst, uint64_t vram_offset, size_t size) {
    (void)dst; (void)vram_offset; (void)size;
}

// All older API
void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (active_driver) active_driver->put_pixel(x, y, color);
}
void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (active_driver) active_driver->fill_rect(x, y, w, h, color);
}
void draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                       uint32_t thickness, uint32_t color) {
    if (active_driver) active_driver->draw_rect_outline(x, y, w, h, thickness, color);
}
void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    if (active_driver) active_driver->draw_line(x0, y0, x1, y1, color);
}
void draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    if (active_driver) active_driver->draw_circle(cx, cy, r, color);
}
void fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    if (active_driver) active_driver->fill_circle(cx, cy, r, color);
}
void set_fg(uint32_t color) {
    if (active_driver) active_driver->set_fg(color);
}
void set_bg(uint32_t color) {
    if (active_driver) active_driver->set_bg(color);
}
void draw_char(uint32_t x, uint32_t y, char c, uint32_t scale) {
    if (active_driver) active_driver->draw_char(x, y, c, scale);
}
void draw_string(uint32_t x, uint32_t y, const char *str, uint32_t scale) {
    if (active_driver) active_driver->draw_string(x, y, str, scale);
}
uint32_t screen_width(void) {
    return active_driver ? active_driver->get_width() : 0;
}
uint32_t screen_height(void) {
    return active_driver ? active_driver->get_height() : 0;
}
uint32_t get_fg(void) {
    return active_driver ? active_driver->get_fg() : 0xFFFFFFFF;
}
uint32_t get_bg(void) {
    return active_driver ? active_driver->get_bg() : 0xFF0D1117;
}
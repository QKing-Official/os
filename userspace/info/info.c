#include "../../libraries/draw.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/timer.h"
#include "../userspace.h"

// Libs since no libc (Yeah I know I should really add it)
static void int_to_str(int n, char *out) {
    if (n == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    char temp[16];
    int i = 0, neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    while (n) { temp[i++] = '0' + (n % 10); n /= 10; }
    int j = 0;
    if (neg) out[j++] = '-';
    while (i > 0) out[j++] = temp[--i];
    out[j] = '\0';
}


// Bits calculation
static void format_size(uint64_t bytes, char *out) {
    const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    uint64_t divisor = 1;
    uint64_t val = bytes;

    while (val >= 1024 && idx < 4) {
        val /= 1024;
        divisor *= 1024;
        idx++;
    }

    uint64_t remainder = bytes % (divisor * 1024);
    uint64_t rounded = val;
    if (remainder >= (divisor * 1024) / 2) {
        rounded++;
        // Handle overflow to next unit
        if (rounded >= 1024 && idx < 4) {
            rounded = 1;
            idx++;
        }
    }

    char num_buf[16];
    int_to_str((int)rounded, num_buf);

    int pos = 0;
    while (num_buf[pos]) {
        out[pos] = num_buf[pos];
        pos++;
    }
    int i = 0;
    while (suffixes[idx][i]) {
        out[pos++] = suffixes[idx][i];
        i++;
    }
    out[pos] = '\0';
}

void info_main(void) {
    set_bg(0xFF0D1117);
    fill_rect(0, 0, screen_width(), screen_height(), get_bg());

    draw_string(20, 20, "SYSTEM INFORMATION (GPU)", 2);

    const gpu_info_t *gpu = gpu_get_info();
    int hw_accel = gpu_is_hardware_accelerated();

    char line[128];
    int y = 70;

    // Vendor
    const char *vendor_str = "Unknown";
    switch (gpu->vendor) {
        case GPU_VENDOR_INTEL:  vendor_str = "Intel"; break;
        case GPU_VENDOR_AMD:    vendor_str = "AMD"; break;
        case GPU_VENDOR_NVIDIA: vendor_str = "NVIDIA"; break;
        case GPU_VENDOR_VMWARE: vendor_str = "VMware"; break;
        case GPU_VENDOR_VIRTIO: vendor_str = "VirtIO"; break;
        default: break;
    }
    draw_string(20, y, "Vendor:", 1);
    draw_string(200, y, vendor_str, 1);
    y += 20;

    // Model
    draw_string(20, y, "Model:", 1);
    draw_string(200, y, gpu->model_str, 1);
    y += 20;

    // PCI IDs
    char id_buf[32];
    draw_string(20, y, "PCI IDs:", 1);
    int_to_str(gpu->vendor_id, id_buf);
    draw_string(200, y, "Vendor 0x", 1);
    draw_string(260, y, id_buf, 1);
    draw_string(340, y, "Device 0x", 1);
    int_to_str(gpu->device_id, id_buf);
    draw_string(420, y, id_buf, 1);
    y += 20;

    // VRAM size
    draw_string(20, y, "VRAM size:", 1);
    char size_buf[32];
    format_size(gpu->vram_size, size_buf);
    draw_string(200, y, size_buf, 1);
    y += 20;

    // Free VRAM
    draw_string(20, y, "VRAM free:", 1);
    uint64_t free_vram = gpu_get_free_vram();
    format_size(free_vram, size_buf);
    draw_string(200, y, size_buf, 1);
    y += 20;

    // Used VRAM
    draw_string(20, y, "VRAM used:", 1);
    uint64_t used_vram = gpu_get_used_vram();
    format_size(used_vram, size_buf);
    draw_string(200, y, size_buf, 1);
    y += 20;

    // Hardware acceleration
    draw_string(20, y, "Acceleration:", 1);
    draw_string(200, y, hw_accel ? "YES (hardware GPU)" : "NO (software framebuffer)", 1);
    y += 30;

    // Framebuffer info
    draw_string(20, y, "Framebuffer:", 1);
    y += 20;
    char buf[64];
    draw_string(40, y, "Width:", 1);
    int_to_str(screen_width(), buf);
    draw_string(120, y, buf, 1);
    draw_string(200, y, "Height:", 1);
    int_to_str(screen_height(), buf);
    draw_string(280, y, buf, 1);
    y += 20;

    draw_string(20, y, "Press Q to exit", 1);

    keyboard_init();
    while (1) {
        if (keyboard_has_key()) {
            char c = keyboard_read();
            if (c == 'q' || c == 'Q')
                break;
        }
        timer_delay_ms(50);
    }
}

int info_test(void) {
    return 1;
}

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program info_prog = {
    .name = "info",
    .main = info_main,
    .test = info_test
};
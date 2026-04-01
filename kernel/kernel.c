#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "draw.h"
#include "timer.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t requests_start[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t requests_end[] = LIMINE_REQUESTS_END_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

extern void init_main(const char *prog_name);

// Minimal strlen not lazy at all!
static size_t k_strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

// Minimal sprintf due me being lazy
static void k_sprintf_timezone(char *buf, int tz) {
    buf[0] = 'U';
    buf[1] = 'T';
    buf[2] = 'C';
    if (tz >= 0) {
        buf[3] = '+';
        buf[4] = '0' + tz;
        buf[5] = 0;
    } else {
        buf[3] = '-';
        buf[4] = '0' - tz;
        buf[5] = 0;
    }
}

// Minimal sprintf for formatting time
static void k_sprintf_time(char *buf, uint8_t h, uint8_t m, uint8_t s) {
    buf[0] = '0' + h / 10;
    buf[1] = '0' + h % 10;
    buf[2] = ':';
    buf[3] = '0' + m / 10;
    buf[4] = '0' + m % 10;
    buf[5] = ':';
    buf[6] = '0' + s / 10;
    buf[7] = '0' + s % 10;
    buf[8] = 0;
}

void kmain(void) {
    // Initialize framebuffer
    draw_init(fb_req.response->framebuffers[0]);
    fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);

    // SPLASHSCREEN
    draw_string(16, 16, "OS Booting...", 2);

    // Initialize RTC timer since I want to see the time
    timer_init();
    timer_set_timezone(0); // Timezone. 0 = UTC(+0)

    // Draw cool letters
    int mid_x = screen_width() / 2 - 32;
    int mid_y = screen_height() / 2 - 32;
    draw_string(mid_x, mid_y, "O", 6);
    draw_string(mid_x + 48, mid_y, "S", 6);

    // Draw timezone
    char tz_buf[32];
    int tz = 0;
    k_sprintf_timezone(tz_buf, tz);
    draw_string(screen_width() - 8 * k_strlen(tz_buf) * 2, screen_height() - 20, tz_buf, 2);

    // Display the time
    uint8_t h, m, s;
    timer_get_time(&h, &m, &s);
    char time_buf[32];
    k_sprintf_time(time_buf, h, m, s);
    draw_string(16, screen_height() - 20, time_buf, 2);

    // Wait 5 seconds
    timer_delay_s(5);

    // THIS IS COOL! (Run init with target program drawing)
    init_main("drawing");

    while (1) __asm__("hlt");
}
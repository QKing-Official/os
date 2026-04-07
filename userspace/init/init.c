#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../userspace.h"
#include <stdint.h>
#include <stddef.h>

extern struct userspace_program __start_userspace_programs[];
extern struct userspace_program __stop_userspace_programs[];

static const uint32_t RESULT_X = 16;
static const uint32_t RESULT_Y = 16;
static const uint64_t DELAY_LOOPS = 10000000;

// ---------------------------------------------------------------------------
// PS/2 flush – drain leftover bytes (mouse packets, key-up events, etc.)
// Without this, the first keyboard_read() an app does gets the mouse byte or
// key-release from the click that launched it, making keyboard seem dead.
// ---------------------------------------------------------------------------
static inline uint8_t _inb(uint16_t port) {
    uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port)); return v;
}
static void ps2_flush(void) {
    for (int i = 0; i < 32; i++) {
        if (!(_inb(0x64) & 0x01)) break;
        _inb(0x60);
    }
}

static void report_status(const char *name, int result, uint32_t x, uint32_t y) {
    // Always force known colours – never trust whatever app left behind
    set_fg(0xFFFFFFFF);
    set_bg(0xFF0D1117);
    switch (result) {
        case 1:
            draw_string(x, y, "[OK] ", 2);
            draw_string(x + 5*8*2, y, name, 2);
            break;
        case 0:
            draw_string(x, y, "[FAILED] ", 2);
            draw_string(x + 8*8*2, y, name, 2);
            break;
        default:
            draw_string(x, y, "[UNKNOWN] ", 2);
            draw_string(x + 10*8*2, y, name, 2);
            break;
    }
}

static void busy_wait(uint64_t loops) {
    for (volatile uint64_t i = 0; i < loops; i++) __asm__("nop");
}

void init_main(const char *prog_name) {
    uint32_t x = RESULT_X, y = RESULT_Y;

    // Hard-reset colours BEFORE touching the screen.
    // Old code: fill_rect(..., get_bg()) — if an app trashed bg_color,
    // get_bg() returned garbage and the "clear" filled with the wrong colour.
    set_fg(0xFFFFFFFF);
    set_bg(0xFF0D1117);

    draw_string(x, y, "Userspace Init Starting...", 2);
    y += 20;

    struct userspace_program *prog = NULL;
    for (struct userspace_program *p = __start_userspace_programs;
         p < __stop_userspace_programs; p++) {
        const char *a = p->name, *b = prog_name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == *b) { prog = p; break; }
    }

    if (!prog) {
        draw_string(x, y, "Program not found!", 2);
        return;
    }

    int res = 1;
    if (prog->test) res = prog->test();
    report_status(prog_name, res, x, y);

    busy_wait(DELAY_LOOPS);

    // Clear with a known constant colour, not get_bg()
    fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);

    // Flush PS/2 buffer so app gets a clean keyboard on first read
    ps2_flush();

    prog->main();

    // Flush again so the caller (desktop/shell) doesn't see app's leftovers
    ps2_flush();
}
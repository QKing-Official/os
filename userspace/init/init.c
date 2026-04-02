#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../userspace.h"
#include <stdint.h>
#include <stddef.h>

extern struct userspace_program __start_userspace_programs[];
extern struct userspace_program __stop_userspace_programs[];

static const uint32_t RESULT_X = 16;
static const uint32_t RESULT_Y = 16;

// Reduced drastically - 500000000 nops fired timer/keyboard faults when called from shell
static const uint64_t DELAY_LOOPS = 10000000;

static void report_status(const char *name, int result, uint32_t x, uint32_t y) {
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

    draw_string(x, y, "Userspace Init Starting...", 2);
    y += 20;

    struct userspace_program *prog = NULL;
    for (struct userspace_program *p = __start_userspace_programs; p < __stop_userspace_programs; p++) {
        const char *a = p->name, *b = prog_name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == *b) { prog = p; break; }
    }

    if (!prog) {
        draw_string(x, y, "Program not found!", 2);
        // Return instead of halt so shell can recover and reprint prompt
        return;
    }

    int res = 1;
    if (prog->test) res = prog->test();
    report_status(prog_name, res, x, y);

    // Short wait so user can see the [OK] status before screen clears
    busy_wait(DELAY_LOOPS);

    // Clear screen and launch program
    fill_rect(0, 0, screen_width(), screen_height(), get_bg());
    prog->main();

    // Program returned
}
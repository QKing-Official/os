#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../userspace.h"

void drawing_main(void) {
    set_bg(0xFF0D1117);
    set_fg(0xFFFFFFFF);
    fill_rect(0, 0, screen_width(), screen_height(), get_bg());
    draw_string(50, 50, "Drawing Program Example", 2);
    while (1) __asm__("hlt");
}

int drawing_test(void) {
    // Return 1 for OK
    return 1; // [OK]

    // Return 0 for FAILED
    // return 0; // [FAILED]

    // Return -1 for UNKNOWN
    // return -1; // [UNKNOWN]

    // If your program returns unexpectedly, you can handle WARNING elsewhere
    // return 2; // [WARNING]
}

// Best thing ever, dynamic program definition
__attribute__((used, section(".userspace_programs")))
struct userspace_program drawing_prog = {
    .name = "drawing",
    .main = drawing_main,
    .test = drawing_test
};
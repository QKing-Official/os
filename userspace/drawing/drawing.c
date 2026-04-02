#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../userspace.h"

static uint32_t cursor_x = 100;
static uint32_t cursor_y = 100;

static uint32_t colors[] = {
    0xFFFFFFFF, // white
    0xFFFF0000, // red
    0xFF00FF00, // green
    0xFF0000FF, // blue
    0xFFFFFF00  // yellow
};

static int current_color = 0;

static void draw_cursor(void) {
    fill_rect(cursor_x, cursor_y, 6, 6, 0xFFFFFFFF);
}

void drawing_main(void) {
    set_bg(0xFF0D1117);
    set_fg(0xFFFFFFFF);

    fill_rect(0, 0, screen_width(), screen_height(), get_bg());

    draw_string(20, 20,
        "Drawing: WASD move | SPACE draw | C clear \ Q quit",
        2);

    keyboard_init();

    while (1) {
        char key = keyboard_read();

        if (!key)
            continue;

        // move
        if (key == 'w' && cursor_y > 0) cursor_y -= 4;
        if (key == 's' && cursor_y < screen_height()-6) cursor_y += 4;
        if (key == 'a' && cursor_x > 0) cursor_x -= 4;
        if (key == 'd' && cursor_x < screen_width()-6) cursor_x += 4;

        // draw the actual pixel

        if (key == ' ')
            fill_rect(cursor_x, cursor_y, 4, 4, colors[current_color]);

        // cls

        if (key == 'c')
            fill_rect(0, 0, screen_width(), screen_height(), get_bg());

        // go away

        if (key == 'q')
            while (1) __asm__("hlt");

        draw_cursor();
    }
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
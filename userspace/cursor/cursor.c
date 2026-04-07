#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/mouse.h"
#include "../userspace.h"

// Color palette to cycle through on left-click
static uint32_t colors[] = {
    0xFFFFFFFF,   // white
    0xFFFF0000,   // red
    0xFF00FF00,   // green
    0xFF0000FF,   // blue
    0xFFFFFF00,   // yellow
    0xFFFF00FF,   // magenta
    0xFF00FFFF,   // cyan
    0xFFFF8800,   // orange
};
static const int color_count  = (int)(sizeof(colors) / sizeof(colors[0]));
static int       current_color = 0;

// Arrow-cursor shape
static void draw_cursor(int32_t x, int32_t y, uint32_t color) {
    for (int i = 0; i < 12; i++)
        put_pixel(x, y + i, color);

    for (int i = 0; i < 10; i++)
        put_pixel(x + i, y + i, color);

    for (int i = 0; i < 7; i++)
        put_pixel(x + i, y + 11, color);

    for (int row = 1; row < 11; row++) {
        int max_col = (row < 7) ? row : (11 - row + 6);
        for (int col = 1; col <= max_col && col < 12; col++)
            put_pixel(x + col, y + row, color);
    }
}

static void erase_cursor(int32_t x, int32_t y) {
    fill_rect(x, y, 13, 13, 0xFF0D1117);
}

static void draw_hud(void) {
    fill_rect(screen_width() - 120, 0, 120, 36, 0xFF1A1F2B);
    fill_rect(screen_width() -  50, 8,  20, 20, colors[current_color]);
    draw_rect_outline(screen_width() - 50, 8, 20, 20, 1, 0xFF888888);
    draw_string(screen_width() - 110, 12, "COLOR:", 1);
}

void cursor_main(void) {
    set_bg(0xFF0D1117);
    set_fg(0xFFFFFFFF);
    fill_rect(0, 0, screen_width(), screen_height(), get_bg());

    draw_string(16, 16, "CURSOR DEMO", 2);
    draw_string(16, 40, "Move: mouse  |  Click: change color  |  Q: quit", 1);

    // Initialise the mouse
    mouse_init(screen_width(), screen_height());
    mouse_set_pos(screen_width() / 2, screen_height() / 2);

    draw_hud();

    int32_t prev_x    = screen_width()  / 2;
    int32_t prev_y    = screen_height() / 2;
    bool    was_left  = false;

    draw_cursor(prev_x, prev_y, colors[current_color]);

    while (1) {
        if (keyboard_has_key()) {
            char key = keyboard_read();
            if (key == 'q' || key == 'Q') {
                // Clean up: stop the mouse stream so the shell doesn't
                // receive mouse packets as garbage keyboard input.
                erase_cursor(prev_x, prev_y);
                mouse_deinit();
                return;
            }
        }

        if (!mouse_poll())
            continue;

        const mouse_state_t *m = mouse_get_state();

        bool clicked = m->left && !was_left;
        was_left = m->left;

        if (clicked) {
            current_color = (current_color + 1) % color_count;
            draw_hud();
        }

        // Only repaint if something actually changed, duh
        bool moved = (m->x != prev_x || m->y != prev_y);
        if (!moved && !clicked)
            continue;

        erase_cursor(prev_x, prev_y);
        draw_cursor(m->x, m->y, colors[current_color]);

        prev_x = m->x;
        prev_y = m->y;
    }
}

int cursor_test(void) {
    return 1; // [OK]
}

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program cursor_prog = {
    .name = "cursor",
    .main = cursor_main,
    .test = cursor_test
};
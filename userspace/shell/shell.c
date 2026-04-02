#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/timer.h"
#include "../userspace.h"
#include "../init/init.h"
#include <stdint.h>
#include <stddef.h>

#define SHELL_BUFFER_SIZE 64
#define CHAR_W (8 * 2)
#define CHAR_H (16 * 2)
#define PROMPT "shell> "
#define PROMPT_LEN 7
#define PROMPT_PX  (10 + PROMPT_LEN * CHAR_W)
#define LINE_H     20

static uint32_t shell_cursor_x = 0;
static uint32_t shell_cursor_y = 0;
static char shell_input[SHELL_BUFFER_SIZE];
static int shell_input_len = 0;

static int k_strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *(unsigned char *)a - *(unsigned char *)b;
}

static void k_snprintf_time(char *buf, uint8_t h, uint8_t m, uint8_t s) {
    buf[0] = '0' + h / 10; buf[1] = '0' + h % 10;
    buf[2] = ':';
    buf[3] = '0' + m / 10; buf[4] = '0' + m % 10;
    buf[5] = ':';
    buf[6] = '0' + s / 10; buf[7] = '0' + s % 10;
    buf[8] = 0;
}

static void shell_prompt(void) {
    draw_string(10, shell_cursor_y, PROMPT, 2);
    shell_cursor_x = PROMPT_PX;
}

static void shell_clear(void) {
    fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);
    shell_cursor_y = 10;
    shell_input_len = 0;
    shell_prompt();
}

static void shell_newline(void) {
    shell_cursor_y += LINE_H;
    if (shell_cursor_y > screen_height() - LINE_H * 2)
        shell_clear();
}

static void shell_print(const char *msg) {
    shell_newline();
    draw_string(10, shell_cursor_y, msg, 2);
}

static void shell_execute(const char *cmd) {
    if (cmd[0] == 0) {
        shell_newline();
        shell_prompt();
        return;
    }

    if (k_strcmp(cmd, "cls") == 0) {
        shell_clear();
        return;

    } else if (k_strcmp(cmd, "clock") == 0) {
        uint8_t h, m, s;
        timer_get_time(&h, &m, &s);
        char buf[16];
        k_snprintf_time(buf, h, m, s);
        shell_print(buf);

    } else if (k_strcmp(cmd, "help") == 0) {
        shell_print("Commands: cls, clock, help, exit, <program>");

    } else if (k_strcmp(cmd, "exit") == 0) {
        while (1) __asm__("hlt");

    } else {
        // Use init_main to launch — same path the kernel uses, guaranteed to work
        init_main(cmd);

        // init_main only returns if program returned (it halts on not found)
        fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);
        shell_cursor_y = 10;
        shell_print("[Program returned]");
    }

    shell_newline();
    shell_prompt();
}

static void shell_main(void) {
    shell_clear();
    keyboard_init();

    while (1) {
        char key = keyboard_read();
        if (!key) continue;

        if (key == '\b') {
            if (shell_input_len > 0) {
                shell_input_len--;
                shell_cursor_x -= CHAR_W;
                fill_rect(shell_cursor_x, shell_cursor_y, CHAR_W, CHAR_H, 0xFF0D1117);
            }
        } else if (key == '\n' || key == '\r') {
            shell_input[shell_input_len] = 0;
            shell_newline();
            shell_execute(shell_input);
            shell_input_len = 0;
        } else if (shell_input_len < SHELL_BUFFER_SIZE - 1) {
            shell_input[shell_input_len++] = key;
            char str[2] = {key, 0};
            draw_string(shell_cursor_x, shell_cursor_y, str, 2);
            shell_cursor_x += CHAR_W;
        }
    }
}

static int shell_test(void) { return 1; }

__attribute__((used, section(".userspace_programs")))
struct userspace_program shell_prog = {
    .name = "shell",
    .main = shell_main,
    .test = shell_test
};
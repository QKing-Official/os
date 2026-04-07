// Thanks for all the github projects that helped me make this thing. It somehow works!

#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/timer.h"
#include "../../libraries/power.h"
#include "../userspace.h"
#include "../init/init.h"
#include <stdint.h>
#include <stddef.h>

// Init shell variables
#define SHELL_BUFFER_SIZE 64
#define CHAR_W (8 * 2)
#define CHAR_H (16 * 2)
#define PROMPT "shell> "
#define PROMPT_LEN 7
#define PROMPT_PX  (10 + PROMPT_LEN * CHAR_W)
#define LINE_H     20

// Build up shell from variables
struct shell_state {
    uint32_t cursor_x;
    uint32_t cursor_y;
    int      input_len;
    char     input[SHELL_BUFFER_SIZE];
};

static struct shell_state S;


// Libs since I dont have libc in this os

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

// The shell pormpt itself, defined in a var
static void shell_prompt(void) {
    draw_string(10, S.cursor_y, PROMPT, 2);
    S.cursor_x = PROMPT_PX;
}

// clear (cls)
static void shell_clear(void) {
    fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);
    S.cursor_y = 10;
    S.input_len = 0;
    shell_prompt();
}

// just litterly /n but worse
static void shell_newline(void) {
    S.cursor_y += LINE_H;
    if (S.cursor_y > screen_height() - LINE_H * 2)
        shell_clear();
}

// Print to shell
static void shell_print(const char *msg) {
    shell_newline();
    draw_string(10, S.cursor_y, msg, 2);
}

// HOLY FUCK THIS TOOK LONG
// This can dynamically execute and check for existing programs that are in the os.
// Biggest breakthrough ever!
// Predefined programs are here as well

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
        shell_print("Commands: cls, clock, help, exit, reboot, shutdown, <program>");

    } else if (k_strcmp(cmd, "exit") == 0) {
        while (1) __asm__("hlt"); // YES, WHY EXIT THE SHELL!

    } else if (k_strcmp(cmd, "reboot") == 0) {
        power_reboot();

    } else if (k_strcmp(cmd, "shutdown") == 0) {
        power_shutdown();

    } else {
        init_main(cmd);
        fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);
        S.cursor_y = 10;
        shell_print("[Program returned]");
    }

    shell_newline();
    shell_prompt();
}

// Main function of the shell + command execution
static void shell_main(void) {
    shell_clear();
    keyboard_init();

    while (1) {
        char key = keyboard_read();
        if (!key) continue;

        if (key == '\b') {
            if (S.input_len > 0) {
                S.input_len--;
                S.cursor_x -= CHAR_W;
                fill_rect(S.cursor_x, S.cursor_y, CHAR_W, CHAR_H, 0xFF0D1117);
            }
        } else if (key == '\n' || key == '\r') {
            S.input[S.input_len] = 0;
            shell_newline();
            shell_execute(S.input);
            S.input_len = 0;
        } else if (S.input_len < SHELL_BUFFER_SIZE - 1) {
            S.input[S.input_len++] = key;
            char str[2] = {key, 0};
            draw_string(S.cursor_x, S.cursor_y, str, 2);
            S.cursor_x += CHAR_W;
        }
    }
}

static int shell_test(void) { return 1; }

// Its better than nothing, dont judge me for this thing.
__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program shell_prog = {
    .name = "shell",
    .main = shell_main,
    .test = shell_test
};

// Soon it will be in desktop as well
// Update: It is
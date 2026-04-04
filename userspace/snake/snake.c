#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/timer.h"
#include "../../libraries/power.h"
#include "../userspace.h"

// Grid config
#define CELL      14
#define COLS      42
#define ROWS      30
#define MAX_LEN   (COLS * ROWS)

// Poll the keyboard for actual controll, Thanks due the gpu driver/framebuffer the screen no longer flashes as well
#define KEY_SLICE_MS 8

static uint32_t OFF_X;
static uint32_t OFF_Y;

static uint32_t RAINBOW[] = {
    0xFFFF4444,
    0xFFFF8844,
    0xFFFFFF44,
    0xFF44FF44,
    0xFF44FFFF,
    0xFF4488FF,
    0xFFBB44FF,
};
#define RAINBOW_LEN 7

#define COLOR_FOOD   0xFFFFFFFF
#define COLOR_BG     0xFF0D1117
#define COLOR_GRID   0xFF161B22
#define COLOR_BORDER 0xFF30363D

typedef struct { int x; int y; } Vec2;

static Vec2 snake[MAX_LEN];
static int  snake_len;
static Vec2 dir;
static Vec2 next_dir;
static Vec2 food;
static int  score;
static int  alive;

static uint32_t rng_state = 12345;
static uint32_t rng_next(void) {
    rng_state = rng_state * 1664525 + 1013904223;
    return rng_state;
}

static void place_food(void) {
    for (int attempts = 0; attempts < MAX_LEN * 2; attempts++) {
        int fx = (int)(rng_next() % COLS);
        int fy = (int)(rng_next() % ROWS);
        int ok = 1;
        for (int i = 0; i < snake_len; i++)
            if (snake[i].x == fx && snake[i].y == fy) { ok = 0; break; }
        if (ok) { food.x = fx; food.y = fy; return; }
    }
}

static void draw_cell(int cx, int cy, uint32_t color) {
    fill_rect((int)OFF_X + cx * CELL + 1,
              (int)OFF_Y + cy * CELL + 1,
              CELL - 2, CELL - 2, color);
}

static void snake_draw(void) {
    fill_rect(0, 0, screen_width(), screen_height(), COLOR_BG);

    // Grid
    for (int x = 0; x <= COLS; x++)
        fill_rect((int)OFF_X + x * CELL, (int)OFF_Y, 1, ROWS * CELL, COLOR_GRID);
    for (int y = 0; y <= ROWS; y++)
        fill_rect((int)OFF_X, (int)OFF_Y + y * CELL, COLS * CELL, 1, COLOR_GRID);

    // Borders, dont cross them
    fill_rect((int)OFF_X - 2, (int)OFF_Y - 2, COLS * CELL + 4, 2, COLOR_BORDER);
    fill_rect((int)OFF_X - 2, (int)OFF_Y + ROWS * CELL, COLS * CELL + 4, 2, COLOR_BORDER);
    fill_rect((int)OFF_X - 2, (int)OFF_Y - 2, 2, ROWS * CELL + 4, COLOR_BORDER);
    fill_rect((int)OFF_X + COLS * CELL, (int)OFF_Y - 2, 2, ROWS * CELL + 4, COLOR_BORDER);

    // SSNAKE
    for (int i = 0; i < snake_len; i++) {
        uint32_t color = RAINBOW[i % RAINBOW_LEN];
        if (i > 0) {
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >>  8) & 0xFF;
            uint8_t b = (color >>  0) & 0xFF;
            int factor = 180 + (75 * (snake_len - i)) / snake_len;
            if (factor > 255) factor = 255;
            r = (uint8_t)((r * factor) >> 8);
            g = (uint8_t)((g * factor) >> 8);
            b = (uint8_t)((b * factor) >> 8);
            color = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
        draw_cell(snake[i].x, snake[i].y, color);
    }

    draw_cell(food.x, food.y, COLOR_FOOD);

    draw_string(10, 10, "SNAKE", 2);
    draw_string(10, 30, "SCORE:", 2);

    char buf[16];
    int n = score, idx = 0;
    if (n == 0) { buf[idx++] = '0'; }
    else {
        int tmp[10], t = 0;
        while (n > 0) { tmp[t++] = n % 10; n /= 10; }
        for (int j = t - 1; j >= 0; j--) buf[idx++] = '0' + tmp[j];
    }
    buf[idx] = 0;
    draw_string(10, 50, buf, 2);
    draw_string(10, screen_height() - 28, "WASD move  Q quit", 1);
}

static void snake_step(void) {
    dir = next_dir;
    Vec2 h = { snake[0].x + dir.x, snake[0].y + dir.y };

    if (h.x < 0 || h.x >= COLS || h.y < 0 || h.y >= ROWS) { alive = 0; return; }

    for (int i = 0; i < snake_len - 1; i++)
        if (snake[i].x == h.x && snake[i].y == h.y) { alive = 0; return; }

    int ate = (h.x == food.x && h.y == food.y);
    int nl = snake_len + (ate ? 1 : 0);
    if (nl > MAX_LEN) nl = MAX_LEN;
    for (int i = nl - 1; i > 0; i--) snake[i] = snake[i-1];
    snake[0] = h;
    snake_len = nl;
    if (ate) { score++; place_food(); }
}

static int process_keys(void) {
    while (keyboard_has_key()) {
        char key = keyboard_read();
        if (key == 'q' || key == 'Q') return 1;
        if ((key=='w'||key=='W') && dir.y==0) { next_dir.x= 0; next_dir.y=-1; }
        if ((key=='s'||key=='S') && dir.y==0) { next_dir.x= 0; next_dir.y= 1; }
        if ((key=='a'||key=='A') && dir.x==0) { next_dir.x=-1; next_dir.y= 0; }
        if ((key=='d'||key=='D') && dir.x==0) { next_dir.x= 1; next_dir.y= 0; }
    }
    return 0;
}

static int wait_tick(uint32_t tick_ms) {
    uint32_t elapsed = 0;
    while (elapsed < tick_ms) {
        uint32_t slice = tick_ms - elapsed;
        if (slice > KEY_SLICE_MS) slice = KEY_SLICE_MS;
        timer_delay_ms(slice);
        elapsed += slice;
        if (process_keys()) return 1;
    }
    return 0;
}

static uint32_t game_speed_ms(void) {
    if (score > 35) return 65;
    if (score > 20) return 90;
    if (score > 10) return 110;
    if (score > 5)  return 130;
    return 150;
}

void snake_main(void) {
    set_bg(COLOR_BG);
    keyboard_init();

    OFF_X = (screen_width()  - COLS * CELL) / 2;
    OFF_Y = (screen_height() - ROWS * CELL) / 2;

    snake_len  = 3;
    snake[0].x = COLS/2;   snake[0].y = ROWS/2;
    snake[1].x = COLS/2-1; snake[1].y = ROWS/2;
    snake[2].x = COLS/2-2; snake[2].y = ROWS/2;
    dir.x = 1; dir.y = 0;
    next_dir = dir;
    score = 0;
    alive = 1;
    place_food();

    while (1) {
        snake_step();
        snake_draw();

        if (!alive) {
            int cx = (int)(OFF_X + (COLS * CELL) / 2) - 60;
            int cy = (int)(OFF_Y + (ROWS * CELL) / 2) - 16;
            draw_string(cx, cy,      "GAME OVER", 3);
            draw_string(cx + 8, cy + 30, "Q to exit", 2);

            // Wait for Q or just timeout after 5s
            for (int i = 0; i < 100; i++) {
                timer_delay_ms(50);
                if (process_keys()) return;
            }
            return;
        }

        if (wait_tick(game_speed_ms())) return;
    }
}

int snake_test(void) { return 1; }

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program snake_prog = {
    .name = "snake",
    .main = snake_main,
    .test = snake_test
};
#include "mouse.h"
#include "draw.h"

// The first time I comment actually everything, enjoy!
// The reason for it is that this is that is that this is configurable.
// PS/2 port definitions (shared controller with keyboard)
#define PS2_DATA    0x60    // read data / write data here
#define PS2_STATUS  0x64    // read status register here
#define PS2_CMD     0x64    // write commands here (same port, different direction)

// Status register bits
#define STATUS_OUTPUT_FULL  (1 << 0)   // a byte is waiting to be read from 0x60
#define STATUS_INPUT_FULL   (1 << 1)   // controller is busy — wait before writing
#define STATUS_MOUSE_DATA   (1 << 5)   // the waiting byte came from the aux (mouse) port
                                        // IMPORTANT: always check this before reading,
                                        // otherwise mouse bytes get consumed as scancodes
                                        // and keyboard bytes confuse packet assembly.

// Controller commands (written to 0x64)
#define CTRL_GET_COMPAQ     0x20       // read current controller configuration byte
#define CTRL_SET_COMPAQ     0x60       // write controller configuration byte
#define CTRL_ENABLE_AUX     0xA8       // enable the auxiliary (mouse) PS/2 port
#define CTRL_DISABLE_AUX    0xA7       // disable the auxiliary PS/2 port
#define CTRL_WRITE_MOUSE    0xD4       // route the next data byte to the mouse

// Mouse device commands (sent through CTRL_WRITE_MOUSE → 0x60)
#define MOUSE_RESET          0xFF
#define MOUSE_ENABLE_STREAM  0xF4      // start sending movement packets automatically
#define MOUSE_DISABLE_STREAM 0xF5      // stop sending packets (use before deinit)
#define MOUSE_SET_DEFAULTS   0xF6
#define MOUSE_SET_SAMPLE     0xF3      // followed by sample-rate byte
#define MOUSE_GET_ID         0xF2      // mouse responds with its type ID
#define MOUSE_SET_RES        0xE8      // followed by resolution byte (0-3)

// Mouse response bytes
#define MOUSE_ACK       0xFA           // sent after every accepted command
#define MOUSE_RESET_OK  0xAA           // sent after a successful self-test

// Compaq controller config byte bit masks
#define COMPAQ_IRQ12_EN       (1 << 1) // route mouse interrupts to IRQ12
#define COMPAQ_MOUSE_CLK_DIS  (1 << 5) // 1 = clock disabled; clear this to enable mouse

// Internal states
static mouse_state_t state;
static int32_t screen_w = 800;
static int32_t screen_h = 600;

// Packet assembler
static uint8_t packet[4];
static uint8_t packet_idx  = 0;
static uint8_t packet_size = 3;   // forced to 3 for reliable VM behaviour

static int32_t cursor_prev_x = 0;
static int32_t cursor_prev_y = 0;

// Low-level I/O helpers
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

static void ps2_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout-- && (inb(PS2_STATUS) & STATUS_INPUT_FULL))
        io_wait();
}

static void ps2_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout-- && !(inb(PS2_STATUS) & STATUS_OUTPUT_FULL))
        io_wait();
}

static uint8_t ps2_read(void) {
    ps2_wait_read();
    return inb(PS2_DATA);
}

static void ps2_write_cmd(uint8_t cmd) {
    ps2_wait_write();
    outb(PS2_CMD, cmd);
}

static void mouse_write(uint8_t byte) {
    ps2_write_cmd(CTRL_WRITE_MOUSE);
    ps2_wait_write();
    outb(PS2_DATA, byte);
}

static bool mouse_cmd(uint8_t cmd) {
    mouse_write(cmd);
    return ps2_read() == MOUSE_ACK;
}

static bool mouse_cmd_data(uint8_t cmd, uint8_t data) {
    if (!mouse_cmd(cmd)) return false;
    mouse_write(data);
    return ps2_read() == MOUSE_ACK;
}

static uint8_t detect_mouse_id(void) {
    mouse_cmd_data(MOUSE_SET_SAMPLE, 200);
    mouse_cmd_data(MOUSE_SET_SAMPLE, 100);
    mouse_cmd_data(MOUSE_SET_SAMPLE,  80);

    mouse_cmd(MOUSE_GET_ID);
    return ps2_read();
}

static void ps2_drain(void) {
    // Give the controller a moment to push everything out, then read until empty
    for (int i = 0; i < 64; i++) {
        if (!(inb(PS2_STATUS) & STATUS_OUTPUT_FULL))
            break;
        inb(PS2_DATA);
        io_wait();
    }
}

// Public API — init
void mouse_init(int32_t w, int32_t h) {
    screen_w = w;
    screen_h = h;

    state.x = w / 2;
    state.y = h / 2;
    cursor_prev_x = state.x;
    cursor_prev_y = state.y;

    ps2_write_cmd(CTRL_ENABLE_AUX);
    ps2_drain();   // extra flush to remove stale bytes

    ps2_write_cmd(CTRL_GET_COMPAQ);
    uint8_t config = ps2_read();
    config |=  COMPAQ_IRQ12_EN;
    config &= ~COMPAQ_MOUSE_CLK_DIS;
    ps2_write_cmd(CTRL_SET_COMPAQ);
    ps2_wait_write();
    outb(PS2_DATA, config);

    mouse_cmd(MOUSE_RESET);
    ps2_read();  // 0xAA — self-test OK
    ps2_read();  // 0x00 — device ID after reset

    mouse_cmd(MOUSE_SET_DEFAULTS);

    // Optional: detect ID but force packet_size to 3 for compatibility
    uint8_t id = detect_mouse_id();
    state.mouse_id = id;
    packet_size = 3;   // force 3-byte mode (ignore wheel to avoid desync)

    mouse_cmd_data(MOUSE_SET_RES,    0x02);  // 4 counts/mm
    mouse_cmd_data(MOUSE_SET_SAMPLE, 60);    // 60 Hz

    mouse_cmd(MOUSE_ENABLE_STREAM);

    ps2_drain();

    packet_idx = 0;
}

// Public API — graceful shutdown
void mouse_deinit(void) {
    // Ask the mouse to stop sending packets
    mouse_cmd(MOUSE_DISABLE_STREAM);

    ps2_write_cmd(CTRL_DISABLE_AUX);

    // Flush whatever was already queued up
    ps2_drain();

    packet_idx = 0;
}

// Public API for polling mode
bool mouse_poll(void) {
    uint8_t status = inb(PS2_STATUS);

    // Nothing in the buffer at all = come back later
    if (!(status & STATUS_OUTPUT_FULL))
        return false;

    if (!(status & STATUS_MOUSE_DATA))
        return false;

    uint8_t byte = inb(PS2_DATA);

    // Invalid first byte: reset assembly and discard
    if (packet_idx == 0 && !(byte & 0x08)) {
        packet_idx = 0;
        return false;
    }

    packet[packet_idx++] = byte;

    if (packet_idx < packet_size)
        return false;

    // Full packet received —> parse it
    packet_idx = 0;

    uint8_t flags = packet[0];
    int32_t dx = (int32_t)(int8_t)packet[1];
    int32_t dy = (int32_t)(int8_t)packet[2];
    if (flags & 0x10) dx = (int32_t)((uint32_t)packet[1] | 0xFFFFFF00u);
    if (flags & 0x20) dy = (int32_t)((uint32_t)packet[2] | 0xFFFFFF00u);

    dy = -dy;   // screen Y direction

    // ----- FIX: direct 1:1 movement (no scaling, no acceleration) -----
    state.x += dx;
    state.y += dy;
    // -----------------------------------------------------------------

    state.delta_x = (int8_t)dx;
    state.delta_y = (int8_t)dy;

    // Clamp to screen bounds
    if (state.x < 0)          state.x = 0;
    if (state.y < 0)          state.y = 0;
    if (state.x >= screen_w)  state.x = screen_w - 1;
    if (state.y >= screen_h)  state.y = screen_h - 1;

    state.buttons = flags & 0x07;
    state.left    = (flags & MOUSE_LEFT)   != 0;
    state.right   = (flags & MOUSE_RIGHT)  != 0;
    state.middle  = (flags & MOUSE_MIDDLE) != 0;

    state.scroll = 0;
    if (packet_size == 4) {
        int8_t z = (int8_t)(packet[3] & 0x0F);
        if (z & 0x08) z |= (int8_t)0xF0;
        state.scroll = z;
    }

    return true;
}

// Public API
const mouse_state_t *mouse_get_state(void) {
    return &state;
}

void mouse_set_pos(int32_t x, int32_t y) {
    state.x = x;
    state.y = y;
    if (state.x < 0)          state.x = 0;
    if (state.y < 0)          state.y = 0;
    if (state.x >= screen_w)  state.x = screen_w - 1;
    if (state.y >= screen_h)  state.y = screen_h - 1;
}

// Cursor drawing
#define CURSOR_SIZE 8

void mouse_draw_cursor(uint32_t color) {
    cursor_prev_x = state.x;
    cursor_prev_y = state.y;

    int32_t x = state.x;
    int32_t y = state.y;

    // Horizontal arm
    for (int i = -CURSOR_SIZE; i <= CURSOR_SIZE; i++) {
        int32_t px = x + i;
        if (px >= 0 && px < screen_w)
            put_pixel((uint32_t)px, (uint32_t)y, color);
    }

    // Vertical arm (skip centre — already drawn above)
    for (int i = -CURSOR_SIZE; i <= CURSOR_SIZE; i++) {
        if (i == 0) continue;
        int32_t py = y + i;
        if (py >= 0 && py < screen_h)
            put_pixel((uint32_t)x, (uint32_t)py, color);
    }
}

void mouse_erase_cursor(uint32_t bg_color) {
    int32_t x = cursor_prev_x;
    int32_t y = cursor_prev_y;

    for (int i = -CURSOR_SIZE; i <= CURSOR_SIZE; i++) {
        int32_t px = x + i;
        if (px >= 0 && px < screen_w)
            put_pixel((uint32_t)px, (uint32_t)y, bg_color);
    }
    for (int i = -CURSOR_SIZE; i <= CURSOR_SIZE; i++) {
        if (i == 0) continue;
        int32_t py = y + i;
        if (py >= 0 && py < screen_h)
            put_pixel((uint32_t)x, (uint32_t)py, bg_color);
    }
}
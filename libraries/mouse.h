#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Mouse button bitmask flags (match the PS/2 packet byte 0 bits)
// ---------------------------------------------------------------------------
#define MOUSE_LEFT   (1 << 0)
#define MOUSE_RIGHT  (1 << 1)
#define MOUSE_MIDDLE (1 << 2)

// ---------------------------------------------------------------------------
// Snapshot of mouse state — updated by mouse_poll()
// ---------------------------------------------------------------------------
typedef struct {
    int32_t x;          // absolute cursor X (clamped to [0, screen_w-1])
    int32_t y;          // absolute cursor Y (clamped to [0, screen_h-1])
    int8_t  delta_x;    // raw X movement from the last packet
    int8_t  delta_y;    // raw Y movement from the last packet
    int8_t  scroll;     // scroll wheel delta: +1 up, -1 down, 0 none
    uint8_t buttons;    // bitmask: MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE
    bool    left;       // true while left button is held
    bool    right;      // true while right button is held
    bool    middle;     // true while middle button is held
    uint8_t mouse_id;   // 0 = basic PS/2, 3 = IntelliMouse (scroll wheel)
} mouse_state_t;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// One-time initialisation. Call at boot (or when entering a mouse-using app).
// screen_w / screen_h define the clamp bounds for the cursor.
void mouse_init(int32_t screen_w, int32_t screen_h);

// Graceful shutdown: disable the mouse stream so the controller stops
// sending bytes. Call this before returning to the shell / another app so
// that stray mouse packets are not misread as keyboard scancodes.
void mouse_deinit(void);

// Poll for a single byte from the PS/2 buffer and assemble packets.
// Returns true (and updates internal state) when a complete packet arrives.
// Call in a tight loop; it intentionally reads at most one byte per call so
// keyboard polling stays interleaved.
bool mouse_poll(void);

// Read-only pointer to the last fully-parsed state.
const mouse_state_t *mouse_get_state(void);

// Warp the cursor to an absolute position (e.g. to centre it on startup).
void mouse_set_pos(int32_t x, int32_t y);

// Draw / erase a simple crosshair at the current cursor position.
// The erase function repaints the old position with bg_color.
void mouse_draw_cursor(uint32_t color);
void mouse_erase_cursor(uint32_t bg_color);

#endif // MOUSE_H
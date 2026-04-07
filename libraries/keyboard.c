#include "keyboard.h"

// ---------------------------------------------------------------------------
// PS/2 port definitions
// ---------------------------------------------------------------------------
#define PS2_DATA    0x60
#define PS2_STATUS  0x64

// Status register bits
#define STATUS_OUTPUT_FULL  (1 << 0)   // a byte is ready in the output buffer
#define STATUS_MOUSE_DATA   (1 << 5)   // set when that byte came from the mouse
                                        // MUST be clear before we treat a byte
                                        // as a keyboard scancode — otherwise
                                        // mouse movement bytes get misread as
                                        // keypresses and spam the shell.

// ---------------------------------------------------------------------------
// I/O helpers
// ---------------------------------------------------------------------------
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Keyboard mapping
static const char scancode_map[128] = {
    0,
    27,  '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   // left ctrl
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   // left shift
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,   // right shift
    '*',
    0,   // left alt
    ' ',
};

// THE ALL HOLY PUBLIC API!
void keyboard_init(void) {
    // Nothing required for polled mode.
}

// Returns true only when the waiting byte came from the keyboard (bit 5 clear).
//
// BUG FIX: the old implementation checked only STATUS_OUTPUT_FULL (bit 0),
// which is set for both keyboard AND mouse bytes. While the mouse is active
// this caused mouse packets to look like keypresses, spamming the shell after
// the cursor app exited. Now we also verify STATUS_MOUSE_DATA is clear.
// A not funny comment, but those are needed as well.
bool keyboard_has_key(void) {
    uint8_t status = inb(PS2_STATUS);
    return (status & STATUS_OUTPUT_FULL) && !(status & STATUS_MOUSE_DATA);
}

char keyboard_read(void) {
    if (!keyboard_has_key())
        return 0;

    uint8_t scancode = inb(PS2_DATA);

    if (scancode & 0x80)
        return 0;

    if (scancode < sizeof(scancode_map))
        return scancode_map[scancode];

    return 0;
}
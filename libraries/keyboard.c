#include "keyboard.h"

// IO lib in code itself since no libraries other then the basic libs like this keyboard.c

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

// PS/2 port defenition
// TODO: USB support
#define PS2_DATA   0x60
#define PS2_STATUS 0x64

// status bit
#define OUTPUT_BUFFER_FULL 1

// Basic US International keyboard
static const char scancode_map[128] = {
    0,
    27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,
    '\\','z','x','c','v','b','n','m',',','.','/',
    0,
    '*',
    0,
    ' ',
};

void keyboard_init(void) {
    // nothing needed for polling mode, idk why it exist, but it breaks when I remove it.
}

bool keyboard_has_key(void) {
    return inb(PS2_STATUS) & OUTPUT_BUFFER_FULL;
}

char keyboard_read(void) {
    if (!keyboard_has_key())
        return 0;

    uint8_t scancode = inb(PS2_DATA);

    // ignore key releases, only presses count
    if (scancode & 0x80)
        return 0;

    if (scancode < sizeof(scancode_map))
        return scancode_map[scancode];

    return 0;
}
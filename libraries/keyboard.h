#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Keyboard driver polled PS/2 + no IRQ since fuck the IRQ

void keyboard_init(void);
bool keyboard_has_key(void);
char keyboard_read(void);

#endif // KEYBOARD_H
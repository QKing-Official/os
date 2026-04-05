#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void keyboard_init(void);

bool keyboard_has_key(void);

char keyboard_read(void);

#endif

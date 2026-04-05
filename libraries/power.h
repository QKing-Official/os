#ifndef POWER_H
#define POWER_H

#include <stdint.h>

// Halt CPU forever no end to it
void power_halt(void);

// Reboot system
void power_reboot(void);

// Shutdown system (ACPI / vm fallback)
void power_shutdown(void);

#endif

// This file works with power controls for common hypervisors.
// This os is not tested on real hardware sadly so i dont know if that works.
// Have fun ig
#ifndef POWER_H
#define POWER_H

#include <stdint.h>

// Halt CPU forever
void power_halt(void);

// Reboot system
void power_reboot(void);

// Shutdown system (ACPI / vm fallback)
void power_shutdown(void);

#endif
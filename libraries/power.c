#include "power.h"

// IDC, I know I have already added this somewhere
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// outw needed for shutdown ports which send 16-bit values
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// HALT
void power_halt(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

// Reboot logic
void power_reboot(void) {
    uint8_t status;

    do {
        status = inb(0x64);
    } while (status & 0x02);

    outb(0x64, 0xFE);

    // Fallback: JUST FUCKING TRIPLE FAULT IT!
    __asm__ volatile ("lidt (0)");
    __asm__ volatile ("int $0");

    // If somehow still alive shut it down by force!
    power_halt();
}

// Poweroff
void power_shutdown(void) {
    // QEMU / Bochs shutdown
    outw(0x604, 0x2000);

    // VirtualBox shutdown
    outw(0x4004, 0x3400);

    // VMware shutdown
    outw(0xB004, 0x2000);

    // If all fail, halt
    power_halt();
}
// Thanks to the random guy in discord for helping me with this!

#include "timer.h"

// Some definition to make things easier to change later on
#define CMOS_ADDR   0x70
#define CMOS_DATA   0x71
#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS   0x04
#define RTC_REG_A   0x0A
#define RTC_REG_B   0x0B

// PIT (Programmable Interval Timer) ports — used for ms-accurate delays
#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43
#define PIT_FREQUENCY 1193182  // PIT base frequency in Hz

// Libs and some required rtc shit
static int8_t timezone_offset = 0;  // UTC offset in hours

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static uint8_t bcd_to_bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static uint8_t rtc_read(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

// Read the current PIT channel 0 counter value
// We latch it first so both bytes are consistent
static uint16_t pit_read_counter(void) {
    outb(PIT_COMMAND, 0x00); // latch channel 0
    uint8_t lo = inb(PIT_CHANNEL0);
    uint8_t hi = inb(PIT_CHANNEL0);
    return (uint16_t)((hi << 8) | lo);
}

// Public functions of the library
void timer_set_timezone(int8_t offset_hours) {
    timezone_offset = offset_hours;
}

void timer_init(void) {
    while (rtc_read(RTC_REG_A) & 0x80);

    // Set PIT channel 0 to mode 2 (rate generator), full 16-bit count
    // This makes the counter count down continuously from 0xFFFF
    // giving us a reliable free-running counter to measure elapsed time
    outb(PIT_COMMAND, 0x34); // channel 0, lobyte/hibyte, mode 2, binary
    outb(PIT_CHANNEL0, 0xFF); // low byte of reload value
    outb(PIT_CHANNEL0, 0xFF); // high byte of reload value
}

void timer_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    uint8_t h, m, s;

    while (rtc_read(RTC_REG_A) & 0x80);

    s = rtc_read(RTC_SECONDS);
    m = rtc_read(RTC_MINUTES);
    h = rtc_read(RTC_HOURS);

    uint8_t reg_b = rtc_read(RTC_REG_B);
    bool is_bcd = !(reg_b & 0x04); // 24h assumed

    if (is_bcd) {
        s = bcd_to_bin(s);
        m = bcd_to_bin(m);
        h = bcd_to_bin(h);
    }

    // Apply timezone offset
    int h_tz = h + timezone_offset;
    if (h_tz < 0) h_tz += 24;
    if (h_tz >= 24) h_tz -= 24;

    *hours   = (uint8_t)h_tz;
    *minutes = m;
    *seconds = s;
}

void timer_delay_s(uint32_t s) {
    uint8_t h, m, sec_start, sec_now;
    timer_get_time(&h, &m, &sec_start);

    for (uint32_t i = 0; i < s; i++) {
        uint8_t next_sec = sec_start + 1;
        if (next_sec >= 60) next_sec = 0;

        do {
            timer_get_time(&h, &m, &sec_now);
        } while (sec_now != next_sec);

        sec_start = next_sec;
    }
}

// PIT one-shot delay using counter latch polling.
// PIT is running as a free-running rate generator (set up in timer_init).
// We snapshot the counter, compute how many ticks we need to wait,
// then poll until that many ticks have elapsed — handling wraparound.
void timer_delay_ms(uint32_t ms) {
    if (ms == 0) return;

    // PIT counts down at 1193182 Hz
    // ticks_per_ms = 1193182 / 1000 = 1193 ticks per ms
    // We process in 50ms chunks to avoid 32-bit overflow
    while (ms > 0) {
        uint32_t chunk = ms > 50 ? 50 : ms;
        ms -= chunk;

        uint32_t ticks_needed = (PIT_FREQUENCY / 1000) * chunk;

        uint16_t start = pit_read_counter();
        uint32_t elapsed = 0;
        uint16_t prev = start;

        while (elapsed < ticks_needed) {
            uint16_t curr = pit_read_counter();
            // Counter counts DOWN and wraps from 0 back to 0xFFFF
            uint16_t delta;
            if (prev >= curr)
                delta = prev - curr;
            else
                delta = prev + (0xFFFF - curr); // wraparound
            elapsed += delta;
            prev = curr;
        }
    }
}
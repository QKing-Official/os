// Thanks to the random guy in discord for helping me with this!

#include "timer.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71
#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS   0x04
#define RTC_REG_A   0x0A
#define RTC_REG_B   0x0B

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

void timer_set_timezone(int8_t offset_hours) {
    timezone_offset = offset_hours;
}

void timer_init(void) {
    while (rtc_read(RTC_REG_A) & 0x80);
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

    *hours = (uint8_t)h_tz;
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

void timer_delay_ms(uint32_t ms) {
    timer_delay_s((ms + 999) / 1000); // Round up
}
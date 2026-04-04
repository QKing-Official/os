#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

// Timezone offset in hours
void timer_set_timezone(int8_t offset_hours);

// Initialize timer (RTC + PIT for ms delays)
void timer_init(void);

// Get current time adjusted for timezone
void timer_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

// Delay in seconds
void timer_delay_s(uint32_t s);

// Delay in ms (uses PIT, actually precise)
void timer_delay_ms(uint32_t ms);

#endif
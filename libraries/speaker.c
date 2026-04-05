#include "speaker.h"
#include "timer.h"

// PIT base freq
#define PIT_FREQ 1193182

// Default BPM
static uint32_t bpm = 120;

// IO for audio

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// speaker control

void speaker_play(uint32_t frequency) {
    if (frequency == 0) { speaker_stop(); return; }

    uint32_t divisor = PIT_FREQ / frequency;
    if (divisor > 0xFFFF) divisor = 0xFFFF;
    if (divisor == 0)     divisor = 1;

    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)(divisor >> 8));

    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);
}

void speaker_stop(void) {
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & ~0x03);
}

void speaker_beep(uint32_t freq, uint32_t ms) {
    speaker_play(freq);
    timer_delay_ms(ms);
    speaker_stop();
}

// BPM

void speaker_set_bpm(uint32_t new_bpm) {
    if (new_bpm == 0) new_bpm = 120;
    bpm = new_bpm;
}

static uint32_t quarter_ms(void) {
    return 60000 / bpm;
}

//Notes
void speaker_note(uint32_t freq, uint32_t beats) {
    uint32_t total_ms = quarter_ms() * beats;

    uint32_t sound_ms = (total_ms * 9) / 10;
    uint32_t gap_ms   = total_ms - sound_ms;

    if (freq == NOTE_REST) {
        timer_delay_ms(total_ms);
    } else {
        speaker_play(freq);
        timer_delay_ms(sound_ms);
        speaker_stop();
        timer_delay_ms(gap_ms);
    }
}

// Play a note for x ms
void speaker_note_ms(uint32_t freq, uint32_t ms) {
    uint32_t sound_ms = (ms * 9) / 10;
    uint32_t gap_ms   = ms - sound_ms;

    if (freq == NOTE_REST) {
        timer_delay_ms(ms);
    } else {
        speaker_play(freq);
        timer_delay_ms(sound_ms);
        speaker_stop();
        timer_delay_ms(gap_ms);
    }
}

// Strudel like playing
void speaker_play_melody(const Note *melody) {
    for (int i = 0; melody[i].freq != 0 || melody[i].beats != 0; i++) {
        speaker_note(melody[i].freq, melody[i].beats);
    }
}
#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/timer.h"
#include "../../libraries/speaker.h"
#include "../userspace.h"

// For all music I have to thank the people that helped me with this!
// I wrote the song api and they got to make some songs!
// So thanks a lot!

// Megalovania (120 BPM)
static const Note megalovania[] = {
    { NOTE_D4,  1 }, { NOTE_D4,  1 }, { NOTE_D5,  2 },
    { NOTE_A4,  1 }, { NOTE_REST,1 }, { NOTE_GS4, 1 },
    { NOTE_REST,1 }, { NOTE_G4,  1 }, { NOTE_REST,1 },
    { NOTE_F4,  2 }, { NOTE_D4,  1 }, { NOTE_F4,  1 }, { NOTE_G4,  1 },

    { NOTE_C4,  1 }, { NOTE_C4,  1 }, { NOTE_D5,  2 },
    { NOTE_A4,  1 }, { NOTE_REST,1 }, { NOTE_GS4, 1 },
    { NOTE_REST,1 }, { NOTE_G4,  1 }, { NOTE_REST,1 },
    { NOTE_F4,  2 }, { NOTE_D4,  1 }, { NOTE_F4,  1 }, { NOTE_G4,  1 },

    { NOTE_B3,  1 }, { NOTE_B3,  1 }, { NOTE_D5,  2 },
    { NOTE_A4,  1 }, { NOTE_REST,1 }, { NOTE_GS4, 1 },
    { NOTE_REST,1 }, { NOTE_G4,  1 }, { NOTE_REST,1 },
    { NOTE_F4,  2 }, { NOTE_D4,  1 }, { NOTE_F4,  1 }, { NOTE_G4,  1 },

    { NOTE_AS3, 1 }, { NOTE_AS3, 1 }, { NOTE_D5,  2 },
    { NOTE_A4,  1 }, { NOTE_REST,1 }, { NOTE_GS4, 1 },
    { NOTE_REST,1 }, { NOTE_G4,  1 }, { NOTE_REST,1 },
    { NOTE_F4,  2 }, { NOTE_D4,  1 }, { NOTE_F4,  1 }, { NOTE_G4,  1 },
    { 0, 0 }
};

// At Doom's Gate (E1M1) (220 BPM)
static const Note doom_e1m1[] = {
    // Phrase A (main riff, first 2 measures)
    { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_E3,  1 }, { NOTE_E2,  1 },
    { NOTE_E2,  1 }, { NOTE_D3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 },
    { NOTE_C3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_AS2, 1 },
    { NOTE_E2,  1 }, { NOTE_REST,1 }, { NOTE_B2,  1 }, { NOTE_C3,  1 },

    // Phrase A' (variation with DS3/D3 ending)
    { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_E3,  1 }, { NOTE_E2,  1 },
    { NOTE_E2,  1 }, { NOTE_D3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 },
    { NOTE_C3,  1 }, { NOTE_E2,  1 }, { NOTE_DS3, 1 }, { NOTE_REST,1 },
    { NOTE_D3,  1 }, { NOTE_REST,1 }, { NOTE_REST,1 }, { NOTE_REST,1 },

    // Bridge (solo section)
    { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_E3,  1 }, { NOTE_E2,  1 },
    { NOTE_G3,  1 }, { NOTE_REST,1 }, { NOTE_FS3, 1 }, { NOTE_REST,1 },
    { NOTE_F3,  1 }, { NOTE_REST,1 }, { NOTE_DS3, 1 }, { NOTE_E3,  1 },
    { NOTE_REST,1 }, { NOTE_GS2, 1 }, { NOTE_A2,  1 }, { NOTE_C3,  1 },
    { NOTE_REST,1 }, { NOTE_A2,  1 }, { NOTE_C3,  1 }, { NOTE_D3,  1 },

    // Phrase A (return of main riff)
    { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_E3,  1 }, { NOTE_E2,  1 },
    { NOTE_E2,  1 }, { NOTE_D3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 },
    { NOTE_C3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_AS2, 1 },
    { NOTE_E2,  1 }, { NOTE_REST,1 }, { NOTE_B2,  1 }, { NOTE_C3,  1 },

    // Phrase A' (second variation)
    { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_E3,  1 }, { NOTE_E2,  1 },
    { NOTE_E2,  1 }, { NOTE_D3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 },
    { NOTE_C3,  1 }, { NOTE_E2,  1 }, { NOTE_DS3, 1 }, { NOTE_REST,1 },
    { NOTE_D3,  1 }, { NOTE_REST,1 }, { NOTE_REST,1 }, { NOTE_REST,1 },

    // Outro / final resolution
    { NOTE_E2,  1 }, { NOTE_E2,  1 }, { NOTE_E3,  1 }, { NOTE_E2,  1 },
    { NOTE_E2,  1 }, { NOTE_D3,  1 }, { NOTE_E2,  1 }, { NOTE_E2,  1 },
    { NOTE_C3,  2 }, { NOTE_REST,2 },

    // Terminator
    { 0, 0 }
};

// Tetris Theme (Korobeiniki) (220 BPM)
static const Note tetris_theme[] = {
    // Phrase A (main melody)
    { NOTE_E4,  1 }, { NOTE_B3,  1 }, { NOTE_C4,  1 }, { NOTE_D4,  1 },
    { NOTE_C4,  1 }, { NOTE_B3,  1 }, { NOTE_A3,  1 }, { NOTE_A3,  1 },
    { NOTE_C4,  1 }, { NOTE_E4,  1 }, { NOTE_D4,  1 }, { NOTE_C4,  1 },
    { NOTE_B3,  1 }, { NOTE_C4,  1 }, { NOTE_D4,  1 }, { NOTE_E4,  1 },

    // Phrase B (rising sequence)
    { NOTE_C4,  1 }, { NOTE_A3,  1 }, { NOTE_A3,  1 }, { NOTE_REST,1 },
    { NOTE_D4,  1 }, { NOTE_F4,  1 }, { NOTE_A4,  1 }, { NOTE_G4,  1 },
    { NOTE_F4,  1 }, { NOTE_E4,  1 }, { NOTE_D4,  1 }, { NOTE_C4,  1 },
    { NOTE_B3,  1 }, { NOTE_C4,  1 }, { NOTE_D4,  1 }, { NOTE_E4,  1 },

    // Phrase C (descending variation)
    { NOTE_C4,  1 }, { NOTE_D4,  1 }, { NOTE_E4,  1 }, { NOTE_C4,  1 },
    { NOTE_A3,  1 }, { NOTE_A3,  1 }, { NOTE_REST,1 }, { NOTE_D4,  1 },
    { NOTE_F4,  1 }, { NOTE_A4,  1 }, { NOTE_G4,  1 }, { NOTE_F4,  1 },
    { NOTE_E4,  1 }, { NOTE_D4,  1 }, { NOTE_C4,  1 }, { NOTE_B3,  1 },
    { NOTE_C4,  1 }, { NOTE_D4,  1 }, { NOTE_E4,  1 }, { NOTE_C4,  1 },
    { NOTE_A3,  1 }, { NOTE_A3,  1 }, { NOTE_REST,2 },

    // Terminator
    { 0, 0 }
};

// Imperial March (100 BPM)
static const Note imperial_march[] = {
    { NOTE_A3,  2 }, { NOTE_A3,  2 }, { NOTE_A3,  2 },
    { NOTE_F3,  1 }, { NOTE_C4,  1 },
    { NOTE_A3,  2 }, { NOTE_F3,  1 }, { NOTE_C4,  1 }, { NOTE_A3,  4 },

    { NOTE_E4,  2 }, { NOTE_E4,  2 }, { NOTE_E4,  2 },
    { NOTE_F4,  1 }, { NOTE_C4,  1 },
    { NOTE_GS3, 2 }, { NOTE_F3,  1 }, { NOTE_C4,  1 }, { NOTE_A3,  4 },

    { NOTE_A4,  2 }, { NOTE_A3,  1 }, { NOTE_A3,  1 },
    { NOTE_A4,  2 }, { NOTE_GS4, 1 }, { NOTE_G4,  1 },
    { NOTE_FS4, 1 }, { NOTE_F4,  1 }, { NOTE_FS4, 2 },
    { NOTE_REST,1 }, { NOTE_AS3, 1 }, { NOTE_DS4, 2 }, { NOTE_D4,  1 }, { NOTE_CS4, 1 },
    { NOTE_C4,  1 }, { NOTE_B3,  1 }, { NOTE_C4,  2 },
    { NOTE_REST,1 }, { NOTE_F3,  1 }, { NOTE_GS3, 2 }, { NOTE_F3,  1 },
    { NOTE_A3,  1 }, { NOTE_C4,  2 }, { NOTE_A3,  1 }, { NOTE_C4,  1 }, { NOTE_E4,  4 },
    { 0, 0 }
};

// Super Mario Theme (200 BPM)
static const Note mario[] = {
    { NOTE_E5,  1 }, { NOTE_E5,  1 }, { NOTE_REST,1 }, { NOTE_E5,  1 },
    { NOTE_REST,1 }, { NOTE_C5,  1 }, { NOTE_E5,  2 },
    { NOTE_G5,  4 }, { NOTE_G4,  4 },

    { NOTE_C5,  3 }, { NOTE_G4,  2 }, { NOTE_E4,  3 },
    { NOTE_A4,  2 }, { NOTE_B4,  2 }, { NOTE_AS4, 1 }, { NOTE_A4,  2 },
    { NOTE_G4,  1 }, { NOTE_E5,  1 }, { NOTE_G5,  1 },
    { NOTE_A5,  2 }, { NOTE_F5,  1 }, { NOTE_G5,  1 },
    { NOTE_REST,1 }, { NOTE_E5,  2 }, { NOTE_C5,  1 }, { NOTE_D5,  1 }, { NOTE_B4,  3 },

    { NOTE_C5,  3 }, { NOTE_G4,  2 }, { NOTE_E4,  3 },
    { NOTE_A4,  2 }, { NOTE_B4,  2 }, { NOTE_AS4, 1 }, { NOTE_A4,  2 },
    { NOTE_G4,  1 }, { NOTE_E5,  1 }, { NOTE_G5,  1 },
    { NOTE_A5,  2 }, { NOTE_F5,  1 }, { NOTE_G5,  1 },
    { NOTE_REST,1 }, { NOTE_E5,  2 }, { NOTE_C5,  1 }, { NOTE_D5,  1 }, { NOTE_B4,  3 },
    { 0, 0 }
};

// Pokemon Route 1 (180 BPM)
static const Note pokemon_route1[] = {
    { NOTE_D5,  1 }, { NOTE_REST,1 }, { NOTE_D5,  1 }, { NOTE_REST,1 },
    { NOTE_D5,  1 }, { NOTE_REST,1 }, { NOTE_G5,  2 }, { NOTE_REST,1 },
    { NOTE_G5,  1 }, { NOTE_A5,  1 }, { NOTE_G5,  1 }, { NOTE_FS5, 1 },
    { NOTE_E5,  3 }, { NOTE_REST,1 },

    { NOTE_E5,  1 }, { NOTE_REST,1 }, { NOTE_E5,  1 }, { NOTE_REST,1 },
    { NOTE_E5,  1 }, { NOTE_REST,1 }, { NOTE_A5,  2 }, { NOTE_REST,1 },
    { NOTE_A5,  1 }, { NOTE_B5,  1 }, { NOTE_A5,  1 }, { NOTE_G5,  1 },
    { NOTE_FS5, 3 }, { NOTE_REST,1 },

    { NOTE_FS5, 1 }, { NOTE_REST,1 }, { NOTE_FS5, 1 }, { NOTE_REST,1 },
    { NOTE_FS5, 1 }, { NOTE_REST,1 }, { NOTE_B5,  2 }, { NOTE_REST,1 },
    { NOTE_B5,  1 }, { NOTE_CS6, 1 }, { NOTE_B5,  1 }, { NOTE_A5,  1 },
    { NOTE_G5,  2 }, { NOTE_E5,  1 }, { NOTE_D5,  1 },

    { NOTE_G5,  2 }, { NOTE_FS5, 1 }, { NOTE_E5,  1 },
    { NOTE_A5,  2 }, { NOTE_G5,  1 }, { NOTE_FS5, 1 },
    { NOTE_B5,  2 }, { NOTE_A5,  1 }, { NOTE_G5,  1 },
    { NOTE_D6,  4 }, { NOTE_REST,4 },
    { 0, 0 }
};

// Song table
typedef struct {
    const char *title;
    const char *author;
    uint32_t    bpm;
    const Note *notes;
} Song;

static const Song songs[] = {
    { "MEGALOVANIA",    "Toby Fox",     480, megalovania    },
    { "AT DOOM'S GATE", "Bobby Prince", 440, doom_e1m1      },
    { "IMPERIAL MARCH", "John Williams",200, imperial_march },
    { "SUPER MARIO",    "Koji Kondo",   400, mario          },
    { "TETRIS THEME", "Korobeiniki", 160, tetris_theme },
    { "POKEMON ROUTE 1","Junichi M.",   360, pokemon_route1 },
};
#define SONG_COUNT (sizeof(songs) / sizeof(songs[0]))

// Keyboard!
typedef struct { char key; uint32_t freq; const char *label; } PianoKey;

static const PianoKey piano_keys[] = {
    { 'a', NOTE_C4,  "C4"  },
    { 'w', NOTE_CS4, "C#4" },
    { 's', NOTE_D4,  "D4"  },
    { 'e', NOTE_DS4, "D#4" },
    { 'd', NOTE_E4,  "E4"  },
    { 'f', NOTE_F4,  "F4"  },
    { 't', NOTE_FS4, "F#4" },
    { 'g', NOTE_G4,  "G4"  },
    { 'y', NOTE_GS4, "G#4" },
    { 'h', NOTE_A4,  "A4"  },
    { 'u', NOTE_AS4, "A#4" },
    { 'j', NOTE_B4,  "B4"  },
    { 'k', NOTE_C5,  "C5"  },
    { 'o', NOTE_CS5, "C#5" },
    { 'l', NOTE_D5,  "D5"  },
    { 'p', NOTE_DS5, "D#5" },
};
#define PIANO_KEY_COUNT 16

// state-
static int current_song = 0;
static int playing      = 0;
static int mode         = 0;

// Helpers

static void draw_player_ui(void) {
    fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);
    draw_string(10, 10, "MUSIC PLAYER", 3);
    draw_string(screen_width() - 150, 14, "[TAB] piano", 1);

    // Torture
    for (int i = 0; i < SONG_COUNT; i++) {
        int y = 65 + i * 44;
        if (i == current_song)
            fill_rect(8, y - 4, screen_width() - 16, 36, 0xFF161B22);
        draw_string(20, y,      songs[i].title,  2);
        draw_string(20, y + 18, songs[i].author, 1);
    }

    int sy = screen_height() - 72;
    fill_rect(0, sy - 2, screen_width(), 2, 0xFF30363D);

    if (playing) {
        draw_string(10, sy + 6,  "NOW PLAYING:", 1);
        draw_string(10, sy + 20, songs[current_song].title, 2);
    } else {
        draw_string(10, sy + 6, "STOPPED", 2);
    }

    draw_string(10, screen_height() - 20,
        "W/S select   ENTER play   N next   Q quit", 1);
}

// Love the piano but this was torture
static void draw_piano_ui(const char *last_note) {
    fill_rect(0, 0, screen_width(), screen_height(), 0xFF0D1117);
    draw_string(10, 10, "PIANO MODE", 3);
    draw_string(screen_width() - 160, 14, "[TAB] player", 1);

    // White keys
    int wx = 40, wy = 120, ww = 36, wh = 120;
    char wk[] = { 'a','s','d','f','g','h','j','k','l' };
    for (int i = 0; i < 9; i++) {
        fill_rect(wx + i*(ww+2), wy, ww, wh, 0xFFEEEEEE);
        char lbl[2] = { wk[i], 0 };
        draw_string(wx + i*(ww+2) + 12, wy + wh - 18, lbl, 1);
    }

    // Black keys
    int bx[] = { 22, 60, 136, 174, 212 };
    char bk[] = { 'w','e','t','y','u' };
    for (int i = 0; i < 5; i++) {
        fill_rect(wx + bx[i], wy, 24, 72, 0xFF333333);
        char lbl[2] = { bk[i], 0 };
        draw_string(wx + bx[i] + 7, wy + 55, lbl, 1);
    }

    draw_string(10, 270, "LAST NOTE:", 2);
    draw_string(180, 270, last_note ? last_note : "---", 2);

    draw_string(10, screen_height() - 20,
        "ASDFGHJKL white   WETYUOP black   Q quit   TAB player", 1);
}

// Piano mode/Keyboard Mode
static int run_piano(void) {
    const char *last_note = "---";
    draw_piano_ui(last_note);

    while (1) {
        if (!keyboard_has_key()) continue;
        char key = keyboard_read();
        if (!key) continue;

        if (key == 'q' || key == 'Q') { speaker_stop(); return 1; }
        if (key == '\t') { mode = 0; return 0; }

        for (int i = 0; i < PIANO_KEY_COUNT; i++) {
            if (key == piano_keys[i].key) {
                last_note = piano_keys[i].label;
                speaker_beep(piano_keys[i].freq, 120);
                draw_piano_ui(last_note);
                break;
            }
        }
    }
}

// Song player
static int run_player(void) {
    draw_player_ui();

    while (1) {
        if (!keyboard_has_key()) continue;
        char key = keyboard_read();
        if (!key) continue;

        if (key == 'q' || key == 'Q') { speaker_stop(); return 1; }
        if (key == '\t') { mode = 1; return 0; }

        if ((key == 'w' || key == 'W') && current_song > 0) {
            current_song--; playing = 0; speaker_stop(); draw_player_ui();
        }
        if ((key == 's' || key == 'S') && current_song < SONG_COUNT - 1) {
            current_song++; playing = 0; speaker_stop(); draw_player_ui();
        }
        if (key == 'n' || key == 'N') {
            current_song = (current_song + 1) % SONG_COUNT;
            playing = 0; speaker_stop(); draw_player_ui();
        }

        if (key == '\n' || key == '\r') {
            playing = 1;
            draw_player_ui();

            const Note *melody = songs[current_song].notes;
            speaker_set_bpm(songs[current_song].bpm);

            for (int i = 0; melody[i].freq != 0 || melody[i].beats != 0; i++) {
                if (keyboard_has_key()) {
                    char k = keyboard_read();
                    if (k == 'q' || k == 'Q') { speaker_stop(); return 1; }
                    if (k == '\t') { speaker_stop(); mode = 1; playing = 0; return 0; }
                    if (k == 'n' || k == 'N') {
                        current_song = (current_song + 1) % SONG_COUNT;
                        speaker_stop(); break;
                    }
                }
                speaker_note(melody[i].freq, melody[i].beats);
            }

            playing = 0;
            draw_player_ui();
        }
    }
}

// Entry
void music_main(void) {
    keyboard_init();
    mode = 0;
    current_song = 0;
    playing = 0;

    while (1) {
        int quit;
        if (mode == 0)
            quit = run_player();
        else
            quit = run_piano();

        if (quit) return;
    }
}

int music_test(void) { return 1; }

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program music_prog = {
    .name = "music",
    .main = music_main,
    .test = music_test
};
// A desktop for my os! This took long!
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "../../libraries/draw.h"
#include "../../libraries/font.h"
#include "../../libraries/keyboard.h"
#include "../../libraries/mouse.h"
#include "../../libraries/timer.h"
#include "../../libraries/power.h"
#include "../userspace.h"
#include "../init/init.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define STATUS_OUTPUT_FULL  (1 << 0)

static void ps2_flush(void) {
    for (int i = 0; i < 32; i++) {
        if (!(inb(PS2_STATUS) & STATUS_OUTPUT_FULL)) break;
        (void)inb(PS2_DATA);
    }
}

// Bugfixes
static void keyboard_full_reset(void) {
    ps2_flush();

    outb(PS2_DATA, 0xFF);

    int timeout = 100000;
    while (timeout--) {
        if (inb(PS2_STATUS) & STATUS_OUTPUT_FULL) {
            uint8_t resp = inb(PS2_DATA);
            if (resp == 0xAA) break;
        }
    }

    ps2_flush();

    outb(PS2_DATA, 0xF4);
    timeout = 10000;
    while (timeout--) {
        if (inb(PS2_STATUS) & STATUS_OUTPUT_FULL) {
            uint8_t resp = inb(PS2_DATA);
            if (resp == 0xFA) break;
        }
    }

    ps2_flush();
}

// Userspace program list
extern struct userspace_program __start_userspace_programs[];
extern struct userspace_program __stop_userspace_programs[];

// COnfig for layout
#define MAX_WINDOWS     8
#define TASKBAR_H       40
#define TITLEBAR_H      24
#define CLOSE_SZ        16
#define ICON_W          64
#define ICON_H          64

// Styles
#define C_BG            0xFF1A1A2E
#define C_GRID          0xFF1F1F30
#define C_TASKBAR       0xFF16213E
#define C_TASKBAR_LINE  0xFF4A4A8A
#define C_WIN_BG        0xFF0F3460
#define C_WIN_BORDER    0xFF4A4A8A
#define C_TITLEBAR      0xFF162447
#define C_TITLEBAR_TX   0xFFE0E0FF
#define C_TEXT          0xFFE0E0E0
#define C_CLOSE         0xFFE94560
#define C_ACCENT        0xFF533483
#define C_ACCENT2       0xFF4A90D9
#define C_ICON_BG       0xFF0D2137
#define C_ICON_BD       0xFF4A4A8A
#define C_STARTBTN      0xFF533483
#define C_STARTBTN_HI   0xFF6A45A0
#define C_MENU_BG       0xFF0D1B35
#define C_MENU_BD       0xFF4A4A8A
#define C_LAUNCHER_BG   0xFF0A1628
#define C_LAUNCHER_HD   0xFF1A2A50
#define C_ROW_A         0xFF0D1F38
#define C_ROW_B         0xFF0A1830
#define C_TERM_BG       0xFF0D1117
#define C_TERM_TEXT     0xFF00FF88
#define C_TERM_PROMPT   0xFF4A90D9
#define C_TERM_CURSOR   0xFF00FF88
#define C_CLOCK_FACE    0xFF1A2A4A

// Windowed terminal
#define TERM_COLS       48
#define TERM_ROWS       16
#define TERM_BUF        64
#define TERM_INPUT_MAX  60
#define TERM_CW         8
#define TERM_CH         10

typedef struct {
    char lines[TERM_BUF][TERM_COLS+1];
    int  line_count;
    int  scroll;
    char input[TERM_INPUT_MAX+1];
    int  input_len;
    bool active;
} Terminal;

static Terminal term;

static void term_push(const char *txt) {
    int d = term.line_count % TERM_BUF;
    int i = 0;
    while (i < TERM_COLS && txt[i]) { term.lines[d][i]=txt[i]; i++; }
    term.lines[d][i] = '\0';
    term.line_count++;
    int vis = TERM_ROWS - 1;
    if (term.line_count > vis) term.scroll = term.line_count - vis;
}

static void term_exec(const char *cmd);   // forward decl

static void term_key(char k) {
    if (k == '\b') {
        if (term.input_len > 0) term.input[--term.input_len] = '\0';
    } else if (k == '\n' || k == '\r') {
        term.input[term.input_len] = '\0';
        term_exec(term.input);
        term.input_len = 0; term.input[0] = '\0';
    } else if (k >= 32 && k < 127 && term.input_len < TERM_INPUT_MAX) {
        term.input[term.input_len++] = k;
        term.input[term.input_len]   = '\0';
    }
}

typedef struct {
    int  x, y, w, h;
    char title[32];
    bool open;
    bool is_terminal;
    bool is_clock;
} Window;

// Launcher
#define LAUNCHER_W      240
#define LAUNCHER_H      210
#define LAUNCHER_ROW_H  24

// Global states
static Window   wins[MAX_WINDOWS];
static int      drag_win   = -1;
static int      drag_off_x, drag_off_y;
static bool     menu_open  = false;
static bool     quit_req   = false;
static int      focused_win = -1;

typedef struct { bool visible; int scroll; } Launcher;
static Launcher launcher = {false,0};

static bool     dirty   = true;
static uint8_t  last_ss = 255;
static int      prev_mx = -1, prev_my = -1;
static uint32_t saved_fg, saved_bg;

// Program discovery
static int prog_count(void) {
    int n=0;
    for (struct userspace_program *p=__start_userspace_programs; p<__stop_userspace_programs; p++) {
        if (strcmp(p->name,"shell")==0||strcmp(p->name,"desktop")==0) continue;
        n++;
    }
    return n;
}
static struct userspace_program *prog_get(int idx) {
    int n=0;
    for (struct userspace_program *p=__start_userspace_programs; p<__stop_userspace_programs; p++) {
        if (strcmp(p->name,"shell")==0||strcmp(p->name,"desktop")==0) continue;
        if (n==idx) return p;
        n++;
    }
    return NULL;
}

// Window management
static int win_add(const char *title,int x,int y,int w,int h,bool is_term) {
    for (int i=0;i<MAX_WINDOWS;i++) {
        if (!wins[i].open) {
            wins[i].open=true; wins[i].x=x; wins[i].y=y;
            wins[i].w=w; wins[i].h=h; wins[i].is_terminal=is_term;
            wins[i].is_clock=false;
            strncpy(wins[i].title,title,31); wins[i].title[31]='\0';
            dirty=true;
            focused_win = i;
            return i;
        }
    }
    return -1;
}
static void win_close(int i) {
    if (i<0||i>=MAX_WINDOWS) return;
    if (wins[i].is_terminal) term.active=false;
    wins[i].open=false; 
    if (focused_win == i) focused_win = -1;
    dirty=true;
}
static void win_front(int i) {
    if (i<0||i>=MAX_WINDOWS||!wins[i].open) return;
    Window tmp=wins[i];
    for (int j=i;j<MAX_WINDOWS-1;j++) wins[j]=wins[j+1];
    wins[MAX_WINDOWS-1]=tmp;
    focused_win = MAX_WINDOWS-1;
    dirty=true;
}
static int win_find_term(void) {
    for (int i=0;i<MAX_WINDOWS;i++)
        if (wins[i].open&&wins[i].is_terminal) return i;
    return -1;
}
static int win_find_clock(void) {
    for (int i=0;i<MAX_WINDOWS;i++)
        if (wins[i].open&&wins[i].is_clock) return i;
    return -1;
}

// State save/restore
static void state_save(void)    { saved_fg=get_fg(); saved_bg=get_bg(); }
static void state_restore(void) { set_fg(saved_fg);  set_bg(saved_bg);  }

// External app launch = full hardware reset to prevent bugs
static void launch_app(const char *name) {
    menu_open = false;
    launcher.visible = false;
    dirty = true;

    state_save();

    mouse_deinit();
    ps2_flush();
    init_main(name);

    for (volatile int i = 0; i < 1000000; i++) __asm__("nop");

    keyboard_full_reset();
    mouse_init(screen_width(), screen_height());

    ps2_flush();

    state_restore();
    fill_rect(0, 0, screen_width(), screen_height(), C_BG);
    dirty = true;

    int ti = win_find_term();
    if (ti >= 0) {
        win_front(ti);
        term.active = true;
    }
}

// Terminal command execution
static void term_exec(const char *cmd) {
    char echo[TERM_COLS+1];
    snprintf(echo, sizeof(echo), "> %s", cmd);
    term_push(echo);

    if (!cmd[0]) return;

    if (strcmp(cmd, "cls") == 0 || strcmp(cmd, "clear") == 0) {
        term.line_count = 0;
        term.scroll = 0;
        return;
    }
    if (strcmp(cmd, "help") == 0) {
        term_push("Commands: cls, clock, help, exit, reboot, shutdown, <program>");
        return;
    }
    if (strcmp(cmd, "clock") == 0) {
        uint8_t h, m, s;
        timer_get_time(&h, &m, &s);
        char buf[12];
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
        term_push(buf);
        return;
    }
    if (strcmp(cmd, "reboot") == 0) {
        power_reboot();
        return;
    }
    if (strcmp(cmd, "shutdown") == 0) {
        power_shutdown();
        return;
    }
    if (strcmp(cmd, "exit") == 0) {
        int ti = win_find_term();
        if (ti >= 0) win_close(ti);
        return;
    }

    // Launch userspace program
    int found = 0;
    for (struct userspace_program *p = __start_userspace_programs; p < __stop_userspace_programs; p++) {
        if (strcmp(p->name, cmd) == 0) {
            found = 1;
            term_push("[launching...]");
            launch_app(cmd);
            term_push("[Program returned]");
            break;
        }
    }
    if (!found) {
        term_push("Unknown command. Type 'help'.");
    }
}

// Clockgraphics
#define DEG_TO_IDX(deg) ((deg) % 360)
static const int16_t sin_table[360] = {
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 173, 190, 207, 224, 241, 258, 275, 292, 309, 325,
    342, 358, 374, 390, 406, 422, 438, 454, 469, 485, 500, 515, 530, 545, 560, 574, 589, 603, 617,
    631, 644, 658, 671, 684, 697, 710, 722, 735, 747, 759, 770, 782, 793, 804, 815, 826, 836, 846,
    856, 866, 876, 885, 894, 903, 912, 920, 928, 936, 944, 951, 959, 966, 972, 979, 985, 991, 997,
    1002, 1007, 1012, 1017, 1021, 1025, 1029, 1033, 1036, 1039, 1042, 1045, 1047, 1049, 1051, 1052,
    1054, 1055, 1056, 1056, 1056, 1056, 1056, 1055, 1054, 1053, 1052, 1050, 1048, 1046, 1044, 1041,
    1038, 1035, 1032, 1028, 1024, 1020, 1015, 1011, 1006, 1001, 995, 990, 984, 978, 972, 965, 958,
    951, 944, 937, 929, 921, 913, 905, 896, 887, 878, 869, 860, 850, 840, 830, 820, 809, 798, 787,
    776, 765, 753, 742, 730, 718, 706, 693, 681, 668, 655, 642, 629, 615, 602, 588, 574, 560, 546,
    532, 517, 503, 488, 473, 458, 443, 428, 413, 397, 382, 366, 350, 335, 319, 303, 287, 270, 254,
    238, 222, 205, 189, 172, 156, 139, 123, 106, 90, 73, 56, 40, 23, 7, -10, -26, -43, -60, -76,
    -93, -109, -126, -142, -158, -174, -190, -206, -222, -238, -253, -269, -284, -299, -314, -329,
    -344, -358, -373, -387, -401, -415, -428, -442, -455, -468, -481, -494, -506, -518, -530, -542,
    -554, -565, -576, -587, -598, -608, -618, -628, -638, -647, -656, -665, -674, -682, -690, -698,
    -706, -713, -720, -727, -734, -740, -746, -752, -757, -763, -768, -772, -777, -781, -785, -788,
    -792, -795, -797, -800, -802, -804, -806, -807, -808, -809, -810, -810, -810, -810, -809, -808,
    -807, -806, -804, -802, -800, -798, -795, -792, -789, -786, -782, -778, -774, -770, -765, -761,
    -756, -750, -745, -739, -733, -727, -721, -714, -708, -701, -694, -687, -679, -672, -664, -656,
    -648, -639, -631, -622, -613, -604, -595, -585, -576, -566, -556, -546, -536, -526, -515, -505,
    -494, -483, -472, -461, -450, -439, -427, -416, -404, -393, -381, -369, -357, -345, -333, -321,
    -309, -296, -284, -271, -259, -246, -233, -221, -208, -195, -182, -169, -156, -143, -130, -117,
    -104, -91, -78, -65, -52, -39, -26, -13, 0
};

static inline int32_t sin_deg(int deg) {
    deg = deg % 360;
    if (deg < 0) deg += 360;
    return sin_table[deg];
}
static inline int32_t cos_deg(int deg) {
    return sin_deg(deg + 90);
}

// Analog Clock Window
static void draw_clock_hand(int cx, int cy, int length, int thickness, int angle_deg, uint32_t color) {
    int math_angle = angle_deg - 90;
    int x2 = cx + (length * cos_deg(math_angle) / 1024);
    int y2 = cy + (length * sin_deg(math_angle) / 1024);
    for (int t = -thickness/2; t <= thickness/2; t++) {
        int xoff = (t * sin_deg(math_angle)) / 1024;
        int yoff = (t * cos_deg(math_angle)) / 1024;
        draw_line(cx + xoff, cy + yoff, x2 + xoff, y2 + yoff, color);
    }
}

static void draw_clock_content(const Window *w) {
    int cx = w->x + w->w/2;
    int cy = w->y + TITLEBAR_H + (w->h - TITLEBAR_H)/2;
    int radius = (w->w < w->h - TITLEBAR_H) ? w->w/2 - 10 : (w->h - TITLEBAR_H)/2 - 10;
    if (radius < 10) radius = 10;

    draw_circle(cx, cy, radius, C_ACCENT2);
    draw_circle(cx, cy, radius-1, C_CLOCK_FACE);
    
    for (int i = 1; i <= 12; i++) {
        int angle = i * 30;   // 0° = up
        int x1 = cx + ((radius-8) * sin_deg(angle-90) / 1024);
        int y1 = cy + ((radius-8) * cos_deg(angle-90) / 1024);
        int x2 = cx + ((radius-2) * sin_deg(angle-90) / 1024);
        int y2 = cy + ((radius-2) * cos_deg(angle-90) / 1024);
        draw_line(x1, y1, x2, y2, C_TITLEBAR_TX);
    }

    uint8_t hh, mm, ss;
    timer_get_time(&hh, &mm, &ss);

    int hour_angle = (hh % 12) * 30 + mm / 2;
    int hour_len = radius * 512 / 1024;
    draw_clock_hand(cx, cy, hour_len, 6, hour_angle, C_TEXT);

    int minute_angle = mm * 6 + ss / 10;
    int minute_len = radius * 717 / 1024;
    draw_clock_hand(cx, cy, minute_len, 4, minute_angle, C_ACCENT);

    int second_angle = ss * 6;
    int second_len = radius * 819 / 1024;
    draw_clock_hand(cx, cy, second_len, 2, second_angle, C_CLOSE);

    fill_circle(cx, cy, 5, C_TITLEBAR_TX);
    fill_circle(cx, cy, 3, C_WIN_BG);
}

static void open_clock(void) {
    int ci = win_find_clock();
    if (ci >= 0) {
        win_front(ci);
        return;
    }
    int idx = win_add("Analog Clock", 200, 100, 200, 200 + TITLEBAR_H, false);
    if (idx >= 0) {
        wins[idx].is_clock = true;
        win_front(idx);
    }
}

// Drawing functions non draw.h
static void draw_bg(void) {
    fill_rect(0,0,screen_width(),screen_height(),C_BG);
    for (uint32_t x=0;x<screen_width();x+=32)
        for (uint32_t y=0;y<(uint32_t)(screen_height()-TASKBAR_H);y+=32)
            put_pixel(x,y,C_GRID);
}

static void draw_taskbar(void) {
    int sw=screen_width(),sh=screen_height(),ty=sh-TASKBAR_H;
    fill_rect(0,ty,sw,TASKBAR_H,C_TASKBAR);
    fill_rect(0,ty,sw,1,C_TASKBAR_LINE);

    uint32_t btn=menu_open?C_STARTBTN_HI:C_STARTBTN;
    fill_rect(6,ty+6,68,TASKBAR_H-12,btn);
    draw_rect_outline(6,ty+6,68,TASKBAR_H-12,1,C_ACCENT2);
    set_fg(0xFFFFFFFF); set_bg(btn);
    draw_string(14,ty+14,"START",1);

    uint8_t hh,mm,ss; timer_get_time(&hh,&mm,&ss);
    char ts[12]; snprintf(ts,sizeof(ts),"%02u:%02u:%02u",hh,mm,ss);
    set_fg(C_TEXT); set_bg(C_TASKBAR);
    draw_string(sw-72,ty+14,ts,1);

    int bx=82;
    for (int i=0;i<MAX_WINDOWS;i++) {
        if (!wins[i].open) continue;
        if (bx+90>sw-90) break;
        fill_rect(bx,ty+6,90,TASKBAR_H-12,0xFF1A2A50);
        draw_rect_outline(bx,ty+6,90,TASKBAR_H-12,1,C_ACCENT);
        set_fg(C_TEXT); set_bg(0xFF1A2A50);
        draw_string(bx+4,ty+14,wins[i].title,1);
        bx+=96;
    }
}

static void draw_start_menu(void) {
    if (!menu_open) return;
    int sh=screen_height(),mx=6,my=sh-TASKBAR_H-90;
    fill_rect(mx,my,168,90,C_MENU_BG);
    draw_rect_outline(mx,my,168,90,1,C_MENU_BD);
    fill_rect(mx+1,my,166,1,C_ACCENT);
    const char *items[3]={"  Apps","  About","  Exit"};
    for (int i=0;i<3;i++) {
        int iy=my+i*28+4;
        set_fg(C_TEXT); set_bg(C_MENU_BG);
        draw_string(mx+6,iy+8,items[i],1);
        if (i<2) fill_rect(mx+4,iy+26,160,1,0xFF2A2A4A);
    }
}

static void draw_launcher(void) {
    if (!launcher.visible) return;
    int sh=screen_height(),lx=80,ly=sh-TASKBAR_H-LAUNCHER_H;
    int rows=(LAUNCHER_H-LAUNCHER_ROW_H)/LAUNCHER_ROW_H;
    fill_rect(lx,ly,LAUNCHER_W,LAUNCHER_H,C_LAUNCHER_BG);
    draw_rect_outline(lx,ly,LAUNCHER_W,LAUNCHER_H,1,C_ACCENT);
    fill_rect(lx,ly,LAUNCHER_W,LAUNCHER_ROW_H,C_LAUNCHER_HD);
    fill_rect(lx,ly+LAUNCHER_ROW_H,LAUNCHER_W,1,C_ACCENT);
    set_fg(0xFFFFFFFF); set_bg(C_LAUNCHER_HD);
    draw_string(lx+8,ly+7,"Launch Application",1);
    int total=prog_count();
    if (!total) {
        set_fg(C_TEXT); set_bg(C_LAUNCHER_BG);
        draw_string(lx+10,ly+LAUNCHER_ROW_H+10,"No apps found.",1);
        return;
    }
    for (int i=0;i<rows;i++) {
        int pi=launcher.scroll+i; if (pi>=total) break;
        struct userspace_program *p=prog_get(pi); if (!p) break;
        int ry=ly+LAUNCHER_ROW_H+i*LAUNCHER_ROW_H;
        uint32_t rb=(i&1)?C_ROW_A:C_ROW_B;
        fill_rect(lx+1,ry,LAUNCHER_W-2,LAUNCHER_ROW_H,rb);
        set_fg(C_TEXT); set_bg(rb);
        draw_string(lx+14,ry+7,p->name,1);
    }
    set_fg(C_ACCENT2); set_bg(C_LAUNCHER_BG);
    if (launcher.scroll>0) draw_string(lx+LAUNCHER_W-14,ly+LAUNCHER_ROW_H+4,"^",1);
    if (launcher.scroll+rows<total) draw_string(lx+LAUNCHER_W-14,ly+LAUNCHER_H-14,"v",1);
}

static void draw_icon(int x,int y,const char *sym,const char *label) {
    fill_rect(x,y,ICON_W,ICON_H,C_ICON_BG);
    draw_rect_outline(x,y,ICON_W,ICON_H,1,C_ICON_BD);
    set_fg(C_ACCENT2); set_bg(C_ICON_BG);
    draw_string(x+16,y+22,sym,2);
    set_fg(C_TEXT); set_bg(C_BG);
    draw_string(x+4,y+ICON_H+4,label,1);
}
static void draw_icons(void) {
    draw_icon(20, 50, ">_", "Terminal");
    draw_icon(104, 50, " i", "About");
    draw_icon(188, 50, "Q", "Clock");
}

static void draw_terminal_content(const Window *w) {
    int cx=w->x+1, cy=w->y+TITLEBAR_H+2;
    int cw=w->w-2, ch=w->h-TITLEBAR_H-2;
    fill_rect(cx,cy,cw,ch,C_TERM_BG);

    int rows_vis=TERM_ROWS-1;
    for (int r=0;r<rows_vis;r++) {
        int li=term.scroll+r; if (li>=term.line_count) break;
        const char *line=term.lines[li%TERM_BUF];
        int ly2=cy+4+r*(TERM_CH+2);
        if (ly2+TERM_CH>cy+ch-(TERM_CH+6)) break;
        set_fg(C_TERM_TEXT); set_bg(C_TERM_BG);
        draw_string(cx+4,ly2,line,1);
    }

    int iy=cy+ch-TERM_CH-6;
    fill_rect(cx+1,iy-2,cw-2,TERM_CH+4,0xFF111A22);
    set_fg(C_TERM_PROMPT); set_bg(0xFF111A22);
    draw_string(cx+4,iy,"$ ",1);
    set_fg(C_TERM_TEXT); set_bg(0xFF111A22);
    draw_string(cx+4+2*TERM_CW,iy,term.input,1);
    int curx=cx+4+2*TERM_CW+term.input_len*TERM_CW;
    fill_rect(curx,iy,2,TERM_CH,C_TERM_CURSOR);
}

static void draw_window(const Window *w) {
    if (!w->open) return;
    // Shadow
    fill_rect(w->x+6,w->y+6,w->w,w->h,0x55000000);
    fill_rect(w->x,w->y,w->w,w->h,C_WIN_BG);
    // Border: accent colour if focused
    uint32_t border = (focused_win >= 0 && &wins[focused_win] == w) ? C_ACCENT2 : C_WIN_BORDER;
    draw_rect_outline(w->x,w->y,w->w,w->h,1,border);

    uint32_t tbar = w->is_terminal ? 0xFF0D2035 : C_TITLEBAR;
    if (focused_win >= 0 && &wins[focused_win] == w) {
        tbar = C_ACCENT;   // brighter title bar for focused window
    }
    fill_rect(w->x,w->y,w->w,TITLEBAR_H,tbar);
    fill_rect(w->x,w->y+TITLEBAR_H,w->w,1,C_ACCENT);
    set_fg(C_TITLEBAR_TX); set_bg(tbar);
    draw_string(w->x+8,w->y+7,w->title,1);

    if (w->is_terminal && term.active) {
        set_fg(C_TERM_CURSOR); set_bg(tbar);
        draw_string(w->x+w->w-60,w->y+7,"[focus]",1);
    }

    int cbx=w->x+w->w-CLOSE_SZ-4, cby=w->y+4;
    fill_rect(cbx,cby,CLOSE_SZ,CLOSE_SZ,C_CLOSE);
    set_fg(0xFFFFFFFF); set_bg(C_CLOSE);
    draw_string(cbx+4,cby+4,"X",1);

    if (w->is_terminal) {
        draw_terminal_content(w);
    } else if (w->is_clock) {
        draw_clock_content(w);
    } else {
        int tx=w->x+10, ty=w->y+TITLEBAR_H+12;
        set_fg(C_TEXT); set_bg(C_WIN_BG);
        if (strcmp(w->title,"About")==0) {
            draw_string(tx,ty,    "OS Desktop devlog 6",1);
            draw_string(tx,ty+18, "Written in C + Limine",1);
            draw_string(tx,ty+36, "Windowed terminal & clock",1);
            draw_string(tx,ty+54, "App discovery",1);
        } else if (strcmp(w->title,"Welcome")==0) {
            draw_string(tx,ty,    "Welcome to OS!",1);
            draw_string(tx,ty+18, "Click Terminal for shell",1);
            draw_string(tx,ty+36, "START > Apps to launch",1);
            draw_string(tx,ty+54, "Drag title bar to move",1);
        } else {
            draw_string(tx,ty,    "Drag title bar to move",1);
            draw_string(tx,ty+18, "Click X to close",1);
        }
    }
}

static void draw_cursor(int x, int y)
{
    const uint32_t WHITE = 0xFFFFFFFF;
    const uint32_t BLACK = 0xFF000000;
    const uint32_t SHADOW = 0x80000000; // soft shadow

    static const uint8_t cursor[16][16] = {
        {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {2,1,2,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {2,1,1,2,0,0,0,0,0,0,0,0,0,0,0,0},
        {2,1,1,1,2,0,0,0,0,0,0,0,0,0,0,0},
        {2,1,1,1,1,2,0,0,0,0,0,0,0,0,0,0},
        {2,1,1,1,1,1,2,0,0,0,0,0,0,0,0,0},
        {2,1,1,1,1,1,1,2,0,0,0,0,0,0,0,0},
        {2,1,1,1,1,1,1,1,2,0,0,0,0,0,0,0},
        {2,1,1,1,1,1,2,2,2,0,0,0,0,0,0,0},
        {2,1,1,2,1,1,2,0,0,0,0,0,0,0,0,0},
        {2,1,2,0,2,1,1,2,0,0,0,0,0,0,0,0},
        {2,2,0,0,0,2,1,1,2,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,2,2,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };

    for (int j = 0; j < 16; j++)
        for (int i = 0; i < 16; i++)
            if (cursor[j][i])
                put_pixel(x + i + 1, y + j + 1, SHADOW);

    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < 16; i++) {
            if (cursor[j][i] == 1)
                put_pixel(x + i, y + j, WHITE);
            else if (cursor[j][i] == 2)
                put_pixel(x + i, y + j, BLACK);
        }
    }
}
static void render_frame(int mx,int my) {
    set_fg(C_TEXT); set_bg(C_BG);
    draw_bg();
    draw_icons();
    for (int i=0;i<MAX_WINDOWS;i++) draw_window(&wins[i]);
    draw_taskbar();
    draw_start_menu();
    draw_launcher();
    draw_cursor(mx,my);
    dirty=false;
}

// Open terminal window
static void open_terminal(void) {
    int ti=win_find_term();
    if (ti>=0) { win_front(ti); term.active=true; dirty=true; return; }
    memset(&term,0,sizeof(term));
    term_push("OS Terminal  (type 'help')");
    term_push("----------------------------");
    term.active=true;
    int idx=win_add("Terminal",120,60,420,210,true);
    if (idx>=0) win_front(idx);
}

// Click handler
static void handle_click(int mx,int my) {
    int sh=screen_height();

    // Launcher clicks
    if (launcher.visible) {
        int lx=80,ly=sh-TASKBAR_H-LAUNCHER_H;
        if (mx>=lx&&mx<lx+LAUNCHER_W&&my>=ly&&my<ly+LAUNCHER_H) {
            int rows=(LAUNCHER_H-LAUNCHER_ROW_H)/LAUNCHER_ROW_H;
            if (mx>=lx+LAUNCHER_W-20) {
                if (my<ly+LAUNCHER_ROW_H*2&&launcher.scroll>0) { launcher.scroll--;dirty=true;return; }
                if (my>=ly+LAUNCHER_H-LAUNCHER_ROW_H&&launcher.scroll+rows<prog_count()) { launcher.scroll++;dirty=true;return; }
            }
            int row=(my-ly-LAUNCHER_ROW_H)/LAUNCHER_ROW_H;
            if (row>=0) { struct userspace_program *p=prog_get(launcher.scroll+row); if (p){launch_app(p->name);return;} }
            return;
        }
        launcher.visible=false; dirty=true;
    }

    // Start menu
    if (menu_open) {
        int menu_x=6,menu_y=sh-TASKBAR_H-90;
        if (mx>=menu_x&&mx<menu_x+168&&my>=menu_y&&my<menu_y+90) {
            int item=(my-menu_y-4)/28;
            if (item==0) { launcher.visible=true;launcher.scroll=0;menu_open=false;dirty=true; }
            else if (item==1) { win_add("About",160,120,260,180,false);menu_open=false; }
            else if (item==2) { quit_req=true; }
            return;
        }
        menu_open=false; dirty=true;
    }

    // Start button
    if (mx>=6&&mx<74&&my>=sh-TASKBAR_H+6&&my<sh-6) { menu_open=!menu_open;dirty=true;return; }

    // Taskbar window buttons
    int bx=82;
    for (int i=0;i<MAX_WINDOWS;i++) {
        if (!wins[i].open) continue;
        if (bx+90>screen_width()-90) break;
        if (mx>=bx&&mx<bx+90&&my>=sh-TASKBAR_H+6&&my<sh-6) { win_front(i);return; }
        bx+=96;
    }

    // Window controls
    for (int i=MAX_WINDOWS-1;i>=0;i--) {
        if (!wins[i].open) continue;
        Window *w=&wins[i];
        int cbx=w->x+w->w-CLOSE_SZ-4,cby=w->y+4;
        if (mx>=cbx&&mx<cbx+CLOSE_SZ&&my>=cby&&my<cby+CLOSE_SZ) { win_close(i);return; }
        if (mx>=w->x&&mx<w->x+w->w&&my>=w->y&&my<w->y+TITLEBAR_H) {
            drag_win=i;drag_off_x=mx-w->x;drag_off_y=my-w->y;
            win_front(i);
            if (w->is_terminal) term.active=true;
            return;
        }
        if (mx>=w->x&&mx<w->x+w->w&&my>=w->y&&my<w->y+w->h) {
            win_front(i);
            if (w->is_terminal) { term.active=true;dirty=true; }
            return;
        }
    }

    // Desktop icons
    if (mx>=20&&mx<20+ICON_W&&my>=50&&my<50+ICON_H) open_terminal();
    else if (mx>=104&&mx<104+ICON_W&&my>=50&&my<50+ICON_H) win_add("About",250,100,280,200,false);
    else if (mx>=188&&mx<188+ICON_W&&my>=50&&my<50+ICON_H) open_clock();
}

static void handle_drag(int mx,int my) {
    if (drag_win<0||!wins[drag_win].open) return;
    int sw=screen_width(),sh=screen_height();
    int nx=mx-drag_off_x,ny=my-drag_off_y;
    if (nx<0) nx=0; if (ny<0) ny=0;
    if (nx+wins[drag_win].w>sw) nx=sw-wins[drag_win].w;
    if (ny+wins[drag_win].h>sh-TASKBAR_H) ny=sh-TASKBAR_H-wins[drag_win].h;
    if (nx!=wins[drag_win].x||ny!=wins[drag_win].y) { wins[drag_win].x=nx;wins[drag_win].y=ny;dirty=true; }
}

static void handle_keyboard(void) {
    if (!keyboard_has_key()) return;
    char k=keyboard_read(); if (!k) return;
    int ti=win_find_term();
    if (ti>=0 && term.active) {
        if (k==27) { term.active=false;dirty=true;return; }
        term_key(k); dirty=true; return;
    }
    // Global shortcuts for exiting program or desktop
    if (k == 'q' || k == 'Q') {
        quit_req = true;
    }
}

void desktop_main(void) {
    timer_init();
    keyboard_init();
    mouse_init(screen_width(), screen_height());
    mouse_set_pos(screen_width()/2, screen_height()/2);

    set_fg(C_TEXT); set_bg(C_BG);
    fill_rect(0,0,screen_width(),screen_height(),C_BG);
    state_save();

    win_add("Welcome",80,80,280,200,false);

    bool last_left=false;
    while (!quit_req) {
        handle_keyboard();

        if (mouse_poll()) {
            const mouse_state_t *m=mouse_get_state();
            bool click=m->left && !last_left;
            if (!m->left) drag_win=-1;
            if (click) handle_click(m->x,m->y);
            handle_drag(m->x,m->y);
            if (m->x!=prev_mx || m->y!=prev_my) { prev_mx=m->x; prev_my=m->y; dirty=true; }
            last_left=m->left;
        }

        uint8_t hh,mm,ss; timer_get_time(&hh,&mm,&ss);
        if (ss != last_ss) { last_ss=ss; dirty=true; }

        if (dirty)
            render_frame(prev_mx>=0 ? prev_mx : (int)screen_width()/2,
                         prev_my>=0 ? prev_my : (int)screen_height()/2);
    }
    mouse_deinit();
}

static int desktop_test(void) { return 1; }

__attribute__((used,section(".userspace_programs"),aligned(1)))
struct userspace_program desktop_prog = {
    .name="desktop", .main=desktop_main, .test=desktop_test,
};
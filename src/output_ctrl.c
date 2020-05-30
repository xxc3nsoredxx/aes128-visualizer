#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <panel.h>

#include "aesvars.h"
#include "output_ctrl.h"

#define CURSOR_HIDE 0

/* Control flag for using ncurses */
int use_ncurses = 1;

/* Key schedule window */
struct window_s key_sched_win;
/* State window */
struct window_s state_win;
/* Round key window */
struct window_s round_key_win;
/* S-Box window */
struct window_s s_box_win;

/* Backup of cursor state */
int curs_bu;

/**
 * Initializes a window_s object
 * w: pointer to a window_s
 */
void init_win (struct window_s *w,
               int width, int height,
               int x, int y,
               const char *title) {
    w->win = newwin(height, width, y, x);
    w->pan = new_panel(w->win);
    w->title = strdup(title);
    w->width = width;
    w->height = height;
    w->x = x;
    w->y = y;
    wborder(w->win, 0, 0, 0, 0, 0, 0, 0, 0);
    mvwprintw(w->win, 0, 1,
              "%s", w->title);
}

/**
 * Clans up the memory used by a window
 * w: ponter to a window_s
 */
void remove_win (struct window_s *w) {
    free(w->title);
    del_panel(w->pan);
    delwin(w->win);
}

/**
 * Populate the s-box window
 */
void pop_sbox_win () {
    unsigned int cx;
    unsigned int cx2;

    /* Fill in the axes */
    mvwprintw(s_box_win.win, 1, 1,
              "XY _0 _1 _2 _3 _4 _5 _6 _7 _8 _9 _a _b _c _d _e _f");
    mvwprintw(s_box_win.win, 3, 1, "0_");
    mvwprintw(s_box_win.win, 5, 1, "1_");
    mvwprintw(s_box_win.win, 7, 1, "2_");
    mvwprintw(s_box_win.win, 9, 1, "3_");
    mvwprintw(s_box_win.win, 11, 1, "4_");
    mvwprintw(s_box_win.win, 13, 1, "5_");
    mvwprintw(s_box_win.win, 15, 1, "6_");
    mvwprintw(s_box_win.win, 17, 1, "7_");
    mvwprintw(s_box_win.win, 19, 1, "8_");
    mvwprintw(s_box_win.win, 21, 1, "9_");
    mvwprintw(s_box_win.win, 23, 1, "a_");
    mvwprintw(s_box_win.win, 25, 1, "b_");
    mvwprintw(s_box_win.win, 27, 1, "c_");
    mvwprintw(s_box_win.win, 29, 1, "d_");
    mvwprintw(s_box_win.win, 31, 1, "e_");
    mvwprintw(s_box_win.win, 33, 1, "f_");
    mvwaddch(s_box_win.win, 2, 3, ACS_ULCORNER);
    whline(s_box_win.win, ACS_HLINE, s_box_win.width);
    mvwaddch(s_box_win.win, 2, s_box_win.width - 1, ACS_RTEE);
    mvwvline(s_box_win.win, 3, 3, ACS_VLINE, s_box_win.height);
    mvwaddch(s_box_win.win, s_box_win.height - 1, 3, ACS_BTEE);

    /* Fill in the actual bytes */
    for (cx = 0; cx < 16; cx++) {
        for (cx2 = 0; cx2 < 16; cx2++) {
            mvwprintw(s_box_win.win, 3 + (cx * 2), 4 + (cx2 * 3),
                      "%02hhx", *(*(SBOX + cx) + cx2));
        }
    }
}

/**
 * ncurses initialization function
 */
void init_ncurses () {
    initscr();
    noecho();
    curs_bu = curs_set(CURSOR_HIDE);

    /* Create the windows */
    init_win(&key_sched_win,
             11 + 2, 0,
             COLS - (11 + 2), 0,
             "Schedule");
    init_win(&state_win,
            11 + 2, 7 + 2,
            0, 0,
            "State");
    init_win(&round_key_win,
            11 + 2, 7 + 2,
            state_win.x + state_win.width, 0,
            "Round Key");
    init_win(&s_box_win,
            50 + 2, 33 + 2,
            (COLS - (50 + 2)) / 2, (LINES - (33 + 2)) / 2,
            "S-Box");
    pop_sbox_win();

    /* Show the windows */
    show_panel(key_sched_win.pan);
    show_panel(state_win.pan);
    show_panel(round_key_win.pan);
    show_panel(s_box_win.pan);
    update_panels();
    doupdate();
}

/**
 * ncurses cleanup function
 */
void leave_ncurses () {
    /* Delete the windows */
    remove_win(&s_box_win);
    remove_win(&round_key_win);
    remove_win(&state_win);
    remove_win(&key_sched_win);

    curs_set(curs_bu);
    endwin();
}

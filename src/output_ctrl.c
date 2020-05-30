#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <panel.h>

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
            state_win.x + 11 + 2, 0,
            "Round Key");

    /* Show the windows */
    show_panel(key_sched_win.pan);
    show_panel(state_win.pan);
    show_panel(round_key_win.pan);
    update_panels();
    doupdate();
}

/**
 * ncurses cleanup function
 */
void leave_ncurses () {
    /* Delete the windows */
    remove_win(&state_win);
    remove_win(&key_sched_win);
    remove_win(&round_key_win);

    curs_set(curs_bu);
    endwin();
}

#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <panel.h>

#include "aesvars.h"
#include "output_ctrl.h"

#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#define CURSOR_HIDE 0

/* Control flag for using ncurses */
int use_ncurses = 1;
/* Milliseconds to delay for */
const int DELAY_MS = 100;
/* List of operations */
const char *ops [] = {
    "Copy initial key into schedule",
    "Get the previous key",
    "Perform shift row operation",
    "Perform substitute row operation",
    "Add the round constant",
    "Add equivalent key from earlier round",
    "Save key in schedule",
    "Copy bytes into state",
    "Perform substitute bytes operation",
    "Perform mix columns operation",
    "Add round key"
};

/* Key schedule window */
struct window_s key_sched_win;
/* State window */
struct window_s state_win;
/* Round key window */
struct window_s round_key_win;
/* S-Box window */
struct window_s s_box_win;
/* Parameters window */
struct window_s params_win;
/* Operations window */
struct window_s ops_win;
/* Current step window */
struct window_s step_win;

/* Index to the top most schedule element to show */
unsigned int key_sched_top;
/* Number of elements to show */
unsigned int key_sched_count;

/* Operation offsets */
int NO_OP = -1;
int COPY_INIT_KEY_OP = 0;
int GET_PREV_KEY_OP = 1;
int SHIFT_ROW_OP = 2;
int SUB_ROW_OP = 3;
int ADD_ROUND_CONST_OP = 4;
int ADD_EQUIV_KEY_OP = 5;
int SAVE_KEY_OP = 6;
int COPY_INTO_STATE_OP = 7;
int SUB_BYTES_OP = 8;
int MIX_COLS_OP = 9;
int ADD_ROUND_KEY_OP = 10;

/* Backup of cursor state */
int curs_bu;
/* Longest operation string */
size_t max_op_len;
/* Current highlighted operation */
int current_op;

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
    w->width = (width == 0) ? COLS - x : width;
    w->height = (height == 0) ? LINES - y : height;
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
 * Populate the operations list
 */
void pop_ops () {
    unsigned int cx;

    /* Initial max */
    max_op_len = 1;

    /* Write an entry for each operation, tracking the max length */
    for (cx = 0; cx < sizeof(ops) / sizeof(*ops); cx++) {
        max_op_len = MAX(max_op_len, strlen(*(ops + cx)));
        mvwprintw(ops_win.win, 1 + cx, 1,
            "%s", *(ops + cx));
    }

    /* Draw the separator */
    mvwaddch(ops_win.win, 0, max_op_len + 1, ACS_TTEE);
    mvwvline(ops_win.win, 1, max_op_len + 1, ACS_VLINE, ops_win.height - 2);
    mvwaddch(ops_win.win, ops_win.height - 1, max_op_len + 1, ACS_BTEE);
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
    /* Set up the display parameters for the key schedule */
    key_sched_top = 0;
    key_sched_count = 0;
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
    init_win(&params_win,
             32 + 12 + 2, 3 + 2,
             round_key_win.x + round_key_win.width, 0,
             "AES-128 Parameters");
    init_win(&ops_win,
             key_sched_win.x - 1, 0,
             0, state_win.height,
             "Operations");
    pop_ops();
    current_op = NO_OP;
    init_win(&step_win,
             params_win.width, 3,
             params_win.x, params_win.height + 1,
             "Current Step");

    /* Show the windows, except for S-Box */
    hide_panel(s_box_win.pan);
    update_panels();
    doupdate();
}

/**
 * ncurses cleanup function
 */
void leave_ncurses () {
    /* Delete the windows */
    remove_win(&step_win);
    remove_win(&ops_win);
    remove_win(&params_win);
    remove_win(&s_box_win);
    remove_win(&round_key_win);
    remove_win(&state_win);
    remove_win(&key_sched_win);

    curs_set(curs_bu);
    endwin();
}

/**
 * Used to update the key schedule display
 */
void update_schedule () {
    unsigned int cx;
    unsigned int cx2;

    /* Clear the current display */
    for (cx = 0; cx < key_sched_win.height - 2; cx++) {
        mvwprintw(key_sched_win.win, 1 + cx, 1, "           ");
    }

    /* Only display the number of elements requested */
    for (cx = 0; cx < key_sched_count; cx++) {
        for (cx2 = 0; cx2 < BPW; cx2++) {
            mvwprintw(key_sched_win.win, 1 + cx, 1 + (cx2 * 3),
                      "%02hhx", *(*(schedule + key_sched_top + cx) + cx2));
        }
    }
    update_panels();
    doupdate();
}

/**
 * Highlight a new operation
 * op: operation number to highlight
 */
void highlight_op (int op) {
    /* Un-highlight currently highlighted operation */
    if (current_op != NO_OP) {
        mvwchgat(ops_win.win, 1 + current_op, 1, max_op_len,
            A_NORMAL, 0, 0);
    }

    /* Highlight the selected operation */
    if (op != NO_OP) {
        mvwchgat(ops_win.win, 1 + op, 1, max_op_len,
            A_STANDOUT, 0, 0);
    }

    /* Track the highlighted operation */
    current_op = op;

    /* Show highlight */
    update_panels();
    doupdate();
}

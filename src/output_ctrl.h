#ifndef OUTPUT_CTRL_H_20200528_224855
#define OUTPUT_CTRL_H_20200528_224855

#include <curses.h>
#include <panel.h>

/* struct that defines a window and associated elements */
struct window_s {
    WINDOW *win;
    PANEL *pan;
    char *title;
    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int y;
};

extern int use_ncurses;
extern const int DELAY_MS;
extern const char *ops [];

extern struct window_s key_sched_win;
extern struct window_s state_win;
extern struct window_s round_key_win;
extern struct window_s s_box_win;
extern struct window_s params_win;
extern struct window_s ops_win;
extern struct window_s step_win;

extern unsigned int key_sched_top;
extern unsigned int key_sched_count;

extern int NO_OP;
extern int COPY_INIT_KEY_OP;
extern int GET_PREV_KEY_OP;
extern int SHIFT_ROW_OP;
extern int SUB_ROW_OP;
extern int ADD_ROUND_CONST_OP;
extern int ADD_EQUIV_KEY_OP;
extern int SAVE_KEY_OP;

void init_ncurses ();
void leave_ncurses ();
void update_schedule ();
void highlight_op (int op);

#endif /* OUTPUT_CTRL_H_20200528_224855 */

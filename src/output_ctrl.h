#ifndef OUTPUT_CTRL_H_20200528_224855
#define OUTPUT_CTRL_H_20200528_224855

#include <curses.h>
#include <panel.h>

/* struct that defines a window and associated elements */
struct window_s {
    WINDOW *win;
    PANEL *pan;
    char *title;
    int width;
    int height;
    int x;
    int y;
};

extern int use_ncurses;

extern struct window_s key_sched_win;
extern struct window_s state_win;
extern struct window_s round_key_win;
extern struct window_s s_box_win;

void init_ncurses ();
void leave_ncurses ();

#endif /* OUTPUT_CTRL_H_20200528_224855 */

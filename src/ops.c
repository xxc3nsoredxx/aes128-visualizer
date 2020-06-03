#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aesvars.h"
#include "ops.h"
#include "output_ctrl.h"

/**
 * Perfomrs a multiplication by x in the finite field
 * shift left 1, if highest bit set xor with 0x1b
 */
char xtime (char c) {
    return (c & 0x80) ? ((c << 1) & 0xfe) ^ 0x1b : (c << 1) & 0xfe;
}

/**
 * Performs polynomial multiplication in the finite field
 * a: polynomial to multiply with
 * b: polynomial to multiply by
 */
char poly_mult (char a, char b) {
    char ret = 0;
    char cur = a;
    unsigned int cx;

    /* Go through each bit of b, performing incremental xtime's */
    for (cx = 0; cx < 7; cx++) {
        /* if the bit is set, add to total */
        if (b & (1 << cx)) {
            ret ^= cur;
        }

        cur = xtime(cur);
    }

    return ret;
}

/**
 * Performs an xor on an entire word
 * dest: pointer to char[4]
 * src: pointer to char[4]
 */
void xor_word (char *dest, char *src) {
    unsigned int cx;

    for (cx = 0; cx < BPW; cx++) {
        *(dest + cx) = *(dest + cx) ^ *(src + cx);
    }
}

/**
 * Performs an s-box transformation on every byte in a word
 * word: pointer to char[4]
 */
void sub_word (char *word) {
    unsigned int cx;

    for (cx = 0; cx < BPW; cx++) {
        *(word + cx) = sub_byte(*(word + cx));
    }
}

/**
 * Calculates the round constant
 * word [x^i-1, {00}, {00}, {00}], where i starts at 1
 * rcon: pointer to char[4]
 */
void round_constant (char *rcon, unsigned int i) {
    char temp = 0x01;

    memset(rcon, 0, BPW);
    for (i = i - 1; i > 0; i--) {
        temp = xtime(temp);
    }
    *rcon = temp;
}

/**
 * Converts a string into a byte array
 * dest: pointer to char[len * BPW]
 * src: pointer to string of length len * BPW * 2
 * len: number of words to copy
 */
void str_bytes (char *dest, const char *src, unsigned int len) {
    unsigned int cx;
    char conv;

    /* Convert a string into a byte array */
    for (cx = 0; cx < len * BPW; cx++) {
        /* Get the top nybble */
        conv = *(src + (2 * cx));
        if (conv >= '0' && conv <= '9') {
            conv -= '0';
        } else if (conv >= 'a' && conv <= 'f') {
            conv -= 'a';
            conv += 10;
        } else if (conv >= 'A' && conv <= 'F') {
            conv -= 'A';
            conv += 10;
        }
        *(dest + cx) = (conv << 4) & 0xf0;

        /* Get the bottom nybble */
        conv = *(src + (2 * cx) + 1);
        if (conv >= '0' && conv <= '9') {
            conv -= '0';
        } else if (conv >= 'a' && conv <= 'f') {
            conv -= 'a';
            conv += 10;
        } else if (conv >= 'A' && conv <= 'F') {
            conv -= 'A';
            conv += 10;
        }
        *(dest + cx) += conv & 0x0f;
    }
}

/**
 * Performs a key expansion on the given key
 */
void key_expand (const char *keystr) {
    unsigned int cx;
    char key [NK * BPW];
    char temp [BPW];
    char rcon [BPW];

    str_bytes(key, keystr, NK);

    /* Highlight operation */
    if (use_ncurses) {
        highlight_op(COPY_INIT_KEY_OP);
    }
    /* Copy the key into the first part of the schedule */
    for (cx = 0; cx < NK; cx++) {
        memcpy(*(schedule + cx), key + (cx * NK), BPW);
        /* Update the schedule window */
        if (use_ncurses) {
            key_sched_count++;
            update_schedule();
            napms(DELAY_MS);
        }
    }

    /* Expand the key into the schedule */
    for (cx = NK; cx < NB * (NR + 1); cx++) {
        /* Highlight operation */
        if (use_ncurses) {
            highlight_op(GET_PREV_KEY_OP);
        }
        memcpy(temp, *(schedule + cx - 1), BPW);
        update_panels();
        doupdate();
        napms(DELAY_MS);
        /* temp =  */
        if (cx % NK == 0) {
            /* sub_word(shift_row(temp)) xor round_constant(rcon, cx/NK) */
            if (use_ncurses) {
                highlight_op(SHIFT_ROW_OP);
            }
            shift_row(temp, 1);
            update_panels();
            doupdate();
            napms(DELAY_MS);
            if (use_ncurses) {
                highlight_op(SUB_ROW_OP);
            }
            sub_word(temp);
            update_panels();
            doupdate();
            napms(DELAY_MS);
            if (use_ncurses) {
                highlight_op(ADD_ROUND_CONST_OP);
            }
            round_constant(rcon, cx / NK);
            xor_word(temp, rcon);
            update_panels();
            doupdate();
            napms(DELAY_MS);
        } else if (NK > 6 && (cx % NK == 4)) {
            sub_word(temp);
        }
        /* schedule[cx] = schedule[cx-NK] xor temp */
        if (use_ncurses) {
            highlight_op(ADD_EQUIV_KEY_OP);
        }
        xor_word(temp, *(schedule + cx - NK));
        update_panels();
        doupdate();
        napms(DELAY_MS);
        if (use_ncurses) {
            highlight_op(SAVE_KEY_OP);
        }
        memcpy(*(schedule + cx), temp, BPW);
        /* Update the schedule window */
        if (use_ncurses) {
            /* Test whether to scroll or append */
            if (key_sched_count < key_sched_win.height - 2) {
                key_sched_count++;
            } else {
                key_sched_top++;
            }
            update_schedule();
            napms(DELAY_MS);
        }
    }
}

/**
 * Add a round key from the schedule
 * round: round number, used to index into schedule
 */
void add_round_key (unsigned int round) {
    unsigned int cx;
    unsigned int cx2;
    char key [4][4];

    /* Extract the round key */
    for (cx = 0; cx < NB; cx++) {
        for (cx2 = 0; cx2 < BPW; cx2++) {
            /* Transpose in the process */
            *(*(key + cx) + cx2) = *(*(schedule + (round * NB) + cx2) + cx);
        }
    }

    /* Print the round key */
    if (use_ncurses) {
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                mvwprintw(round_key_win.win, 1 + (cx2 * 2), 1 + (cx * 3),
                          "%02hhx", *(*(key + cx) + cx2));
                /* Highlight the byte in the key schedule */
                mvwchgat(key_sched_win.win, 1, 1 + (cx2 * 3), 2,
                         A_STANDOUT, 0, 0);
                update_panels();
                doupdate();
                napms(DELAY_MS);
            }
            if (key_sched_top >= NB * (NR + 1) - (key_sched_win.height - 2)) {
                if (key_sched_count != 0) {
                    key_sched_count--;
                }
            }
            key_sched_top++;
            update_schedule();
        }
    } else {
        printf("Round key:\n");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx ", *(*(key + cx) + cx2));
            }
            printf("\n");
        }
    }

    /* Add it to the state */
    for (cx = 0; cx < NB; cx++) {
        xor_word(*(state + cx), *(key + cx));
    }
}

/**
 * Performs an s-box transformation on a byte
 */
char sub_byte (char byte) {
    unsigned int row = (byte >> 4) & 0x0f;
    unsigned int col = byte & 0x0f;

    return *(*(SBOX + row) + col);
}

/**
 * Shifts a row
 * [a0, a1, a2, a3] -> [a1, a2, a3, a0]
 * row: pointer to char[4]
 * amt: amount to shift
 */
void shift_row (char *row, unsigned int amt) {
    char save;
    for (; amt > 0; amt--) {
        save = *row;
        memmove(row, row + 1, BPW - 1);
        *(row + BPW - 1) = save;
    }
}

/**
 * Performs the column mixing
 * column multiplier: {03}x^3 + {01}x^2 + {01}x + {02}
 * col: the column number to mix
 */
void mix_col (unsigned int col) {
    /* Multiplier polynomials */
    char a0 = 0x02;
    char a3 = 0x03;

    /* Column polynomials */
    char s0 = *(*(state + 0) + col);
    char s1 = *(*(state + 1) + col);
    char s2 = *(*(state + 2) + col);
    char s3 = *(*(state + 3) + col);

    /* New s0 */
    *(*(state + 0) + col) = poly_mult(s0, a0)
                          ^ poly_mult(s1, a3)
                          ^ s2
                          ^ s3;
    /* New s1 */
    *(*(state + 1) + col) = s0
                          ^ poly_mult(s1, a0)
                          ^ poly_mult(s2, a3)
                          ^ s3;
    /* New s2 */
    *(*(state + 2) + col) = s0
                          ^ s1
                          ^ poly_mult(s2, a0)
                          ^ poly_mult(s3, a3);
    /* New s3 */
    *(*(state + 3) + col) = poly_mult(s0, a3)
                          ^ s1
                          ^ s2
                          ^ poly_mult(s3, a0);
}

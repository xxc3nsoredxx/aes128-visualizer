#include <stdlib.h>
#include <string.h>

#include "aesvars.h"
#include "ops.h"

/**
 * Converts a string into a byte array
 * key: pointer to char[NK * BPW]
 * keystr: pointer to string of length NK * BPW * 2
 */
void str_bytes (char *key, const char *keystr) {
    unsigned int cx;
    char conv;

    /* Convert a string into a byte array */
    for (cx = 0; cx < NK * BPW; cx++) {
        /* Get the top nybble */
        conv = *(keystr + (2 * cx));
        if (conv >= '0' && conv <= '9') {
            conv -= '0';
        } else if (conv >= 'a' && conv <= 'f') {
            conv -= 'a';
            conv += 10;
        } else {
            conv -= 'A';
            conv += 10;
        }
        *(key + cx) = (conv << 4) & 0xf0;

        /* Get the bottom nybble */
        conv = *(keystr + (2 * cx) + 1);
        if (conv >= '0' && conv <= '9') {
            conv -= '0';
        } else if (conv >= 'a' && conv <= 'f') {
            conv -= 'a';
            conv += 10;
        } else {
            conv -= 'A';
            conv += 10;
        }
        *(key + cx) += conv & 0x0f;
    }
}

/**
 * Perfomrs a multiplication by x in the finite field
 * shift left 1, if highest bit set xor with 0x1b
 */
char xtime (char c) {
    return (c & 0x80) ? ((c << 1) & 0xfe) ^ 0x1b : (c << 1) & 0xfe;
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
 * Rotates a word
 * [a0, a1, a2, a3] -> [a1, a2, a3, a0]
 * word: pointer to char[4]
 */
void rot_word (char *word) {
    char save = *word;
    memmove(word, word + 1, BPW - 1);
    *(word + BPW - 1) = save;
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
 * Performs a key expansion on the given key
 */
void key_expand (const char *keystr) {
    unsigned int cx;
    char key [NK * BPW];
    char temp [BPW];
    char rcon [BPW];
    unsigned int cx2;

    str_bytes(key, keystr);

    /* Copy the key into the first part of the schedule */
    for (cx = 0; cx < NK; cx++) {
        memcpy(*(schedule + cx), key + (cx * NK), BPW);
    }

    /* Expand the key into the schedule */
    for (cx = NK; cx < NB * (NR + 1); cx++) {
        memcpy(temp, *(schedule + cx - 1), BPW);
        /* temp =  */
        if (cx % NK == 0) {
            /* sub_word(rot_word(temp)) xor round_constant(rcon, cx/NK) */
            rot_word(temp);
            sub_word(temp);
            round_constant(rcon, cx / NK);
            xor_word(temp, rcon);
        } else if (NK > 6 && (cx % NK == 4)) {
            sub_word(temp);
        }
        /* schedule[cx] = schedule[cx-NK] xor temp */
        xor_word(temp, *(schedule + cx - NK));
        memcpy(*(schedule + cx), temp, BPW);
    }
}

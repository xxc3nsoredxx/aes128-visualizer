#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>
#include <panel.h>

#include "aesvars.h"
#include "ops.h"
#include "output_ctrl.h"

const char *optstring = ":hi:k:n";

/* Default values for key and input */
char key[] = "2b7e151628aed2a6abf7158809cf4f3c";
char input[] = "3243f6a8885a308d313198a2e0370734";

void usage () {
    printf("Usage: aes128-visualizer [options]\n");
    printf("    -h          print this help\n");
    printf("    -i data     hex string to use as input, at most 16 bytes\n");
    printf("                    anything longer is truncated\n");
    printf("    -k key      encryption key (128 bits)\n");
    printf("    -n          no ncurses visualization, dump to terminal\n");
}

int main (int argc, char **argv) {
    int opt;
    unsigned int cx;
    unsigned int cx2;
    unsigned int round;

    /* Parse arguments */
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            exit(1);
        case 'i':
            /* Reset input to null bytes */
            memset(input, 0, NB * BPW * 2);
            strncpy(input, optarg, NB * BPW * 2);
            break;
        case 'k':
            /* Reset key to null bytes */
            memset(key, 0, NK * BPW * 2);
            strncpy(key, optarg, NK * BPW * 2);

            /* Test the key length */
            if (strlen(key) != NB * BPW * 2) {
                printf("Key not of 128 bit length!\n");
                usage();
                exit(1);
            }
            break;
        case 'n':
            use_ncurses = 0;
            break;
        /* No argument given */
        case ':':
            printf("Option '%s' requires %s as an argument.\n",
                *(argv + optind - 1),
                (!strcmp(*(argv + optind - 1), "-i")) ? "input data"
                : "a key"
            );
            usage();
            exit(1);
        /* Invalid option */
        case '?':
            printf("Invalid option: '%s'\n", *(argv + optind - 1));
            usage();
            exit(1);
        }
    }

    if (use_ncurses) {
        init_ncurses();
    }

    /* Initialize the schedule */
    schedule = calloc(NB * (NR + 1), sizeof(*schedule));
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        *(schedule + cx) = calloc(BPW, sizeof(**schedule));
    }
    /* Initialize the state */
    state = calloc(NB, sizeof(*state));
    for (cx = 0; cx < NB; cx++) {
        *(state + cx) = calloc(BPW, sizeof(**state));
    }

    /* Create the key schedule */
    key_expand(key);
    /* Print the key schedule */
    if (use_ncurses) {
    } else {
        printf("Key schedule:\n");
        for (cx = 0; cx < NB * (NR + 1); cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx", *(*(schedule + cx) + cx2));
            }
            printf("\n");
        }
    }

    /* Copy input into state */
    for (cx = 0; cx < NB; cx++) {
        str_bytes(*(state + cx), input + (cx * NB * 2), 1);
    }
    /* Transpose state */
    for (cx = 1; cx < NB; cx++) {
        for (cx2 = 0; cx2 < cx; cx2++) {
            *(*(state + cx) + cx2) ^= *(*(state + cx2) + cx);
            *(*(state + cx2) + cx) ^= *(*(state + cx) + cx2);
            *(*(state + cx) + cx2) ^= *(*(state + cx2) + cx);
        }
    }

    /* AES rounds */
    for (round = 0; round < NR + 1; round++) {
        if (use_ncurses) {
        } else {
            printf("Round %u\n", round);
            printf("========\n");
            printf("State:\n");
            for (cx = 0; cx < NB; cx++) {
                for (cx2 = 0; cx2 < BPW; cx2++) {
                    printf("%02hhx ", *(*(state + cx) + cx2));
                }
                printf("\n");
            }
        }

        /* Round 0 only adds key */
        if (round == 0) {
            goto add_key;
        }

        /* Feed the state through the s-box */
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                char *c = *(state + cx) + cx2;
                *c = sub_byte(*c);
            }
        }
        if (use_ncurses) {
        } else {
            printf("After S-Box:\n");
            for (cx = 0; cx < NB; cx++) {
                for (cx2 = 0; cx2 < BPW; cx2++) {
                    printf("%02hhx ", *(*(state + cx) + cx2));
                }
                printf("\n");
            }
        }

        /* Shift the rows */
        for (cx = 1; cx < BPW; cx++) {
            shift_row(*(state + cx), cx);
        }
        if (use_ncurses) {
        } else {
            printf("After Row Shifts:\n");
            for (cx = 0; cx < NB; cx++) {
                for (cx2 = 0; cx2 < BPW; cx2++) {
                    printf("%02hhx ", *(*(state + cx) + cx2));
                }
                printf("\n");
            }
        }

        /* Mix the columns except last round */
        if (round == NR) {
            goto add_key;
        }
        for (cx = 0; cx < NB; cx++) {
            mix_col(cx);
        }
        if (use_ncurses) {
        } else {
            printf("After Mix Columns:\n");
            for (cx = 0; cx < NB; cx++) {
                for (cx2 = 0; cx2 < BPW; cx2++) {
                    printf("%02hhx ", *(*(state + cx) + cx2));
                }
                printf("\n");
            }
        }

add_key:
        /* Add the round key */
        add_round_key(round);

        if (!use_ncurses) {
            /* Blank between rounds */
            printf("\n");
        }
    }

    /* Print the final state */
    if (use_ncurses) {
    } else {
        printf("Final State:\n");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx ", *(*(state + cx) + cx2));
            }
            printf("\n");
        }
        printf("\n");
    }

    /* Print the results */
    if (use_ncurses) {
    } else {
        printf("Plaintext:  %s\n", input);
        printf("Key:        %s\n", key);
        printf("Ciphertext: ");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx", *(*(state + cx2) + cx));
            }
        }
        printf("\n");
    }

    /* Cleanup */
    for (cx = 0; cx < NB; cx++) {
        free(*(state + cx));
    }
    free(state);
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        free(*(schedule + cx));
    }
    free(schedule);
    if (use_ncurses) {
        getch();
        leave_ncurses();
    }
    return 0;
}

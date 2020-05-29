#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "aesvars.h"
#include "ops.h"
#include "output_ctrl.h"

const char *optstring = ":hi:k:n";

/* Default values for key and input */
char key[] = "2b7e151628aed2a6abf7158809cf4f3c";
char input[] = "3243f6a8885a308d313198a2e0370734";

/* Control flag for using ncurses */
int use_curses = 1;

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
            strncpy(input, optarg, 32);
            break;
        case 'k':
            strncpy(key, optarg, 32);
            break;
        case 'n':
            use_curses = 0;
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

    /* Test the key length */
    if (strlen(key) != NB * BPW * 2) {
        printf("Key not of 128 bit length!\n");
        usage();
        exit(1);
    }

    /* Pad the input with null bytes if < 16 bytes long */
    if (strlen(input) < NB * BPW * 2) {
        memset(input + strlen(input), 0, (NB * BPW * 2) - strlen(input));
    }
    printf("Input len: %lu", strlen(input));

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
    printf("Key schedule:\n");
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        for (cx2 = 0; cx2 < BPW; cx2++) {
            printf("%02hhx", *(*(schedule + cx) + cx2));
        }
        printf("\n");
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
        printf("Round %u\n", round);
        printf("========\n");
        printf("State: \n");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx ", *(*(state + cx) + cx2));
            }
            printf("\n");
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
        printf("After S-Box: \n");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx ", *(*(state + cx) + cx2));
            }
            printf("\n");
        }

        /* Shift the rows */
        for (cx = 1; cx < BPW; cx++) {
            shift_row(*(state + cx), cx);
        }
        printf("After Row Shifts: \n");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx ", *(*(state + cx) + cx2));
            }
            printf("\n");
        }

        /* Mix the columns except last round */
        if (round == NR) {
            goto add_key;
        }
        for (cx = 0; cx < NB; cx++) {
            mix_col(cx);
        }
        printf("After Mix Columns: \n");
        for (cx = 0; cx < NB; cx++) {
            for (cx2 = 0; cx2 < BPW; cx2++) {
                printf("%02hhx ", *(*(state + cx) + cx2));
            }
            printf("\n");
        }

add_key:
        /* Add the round key */
        add_round_key(round);

        /* Blank between rounds */
        printf("\n");
    }

    /* Print the final state */
    printf("Final State: \n");
    for (cx = 0; cx < NB; cx++) {
        for (cx2 = 0; cx2 < BPW; cx2++) {
            printf("%02hhx ", *(*(state + cx) + cx2));
        }
        printf("\n");
    }
    printf("\n");

    /* Print the inputs */
    printf("Plaintext:  %s\n", input);
    printf("Key:        %s\n", key);
    /* Output as string */
    printf("Ciphertext: ");
    for (cx = 0; cx < NB; cx++) {
        for (cx2 = 0; cx2 < BPW; cx2++) {
            printf("%02hhx", *(*(state + cx2) + cx));
        }
    }
    printf("\n");

    /* Cleanup */
    for (cx = 0; cx < NB; cx++) {
        free(*(state + cx));
    }
    free(state);
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        free(*(schedule + cx));
    }
    free(schedule);
    return 0;
}

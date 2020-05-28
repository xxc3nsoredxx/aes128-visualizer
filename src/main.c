#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aesvars.h"
#include "ops.h"

const char *key = "2b7e151628aed2a6abf7158809cf4f3c";
const char *input = "3243f6a8885a308d313198a2e0370734";

int main () {
    unsigned int cx;
    unsigned int cx2;

    /* Initialize the schedule */
    schedule = calloc(NB * (NR + 1), sizeof(*schedule));
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        *(schedule + cx) = calloc(BPW, sizeof(**schedule));
    }

    key_expand(key);

    /* Print the key schedule */
    printf("Key schedule:\n");
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        for (cx2 = 0; cx2 < BPW; cx2++) {
            printf("%02hhx", *(*(schedule + cx) + cx2));
        }
        printf("\n");
    }

    /* Cleanup */
    for (cx = 0; cx < NB * (NR + 1); cx++) {
        free(*(schedule + cx));
    }
    free(schedule);
    return 0;
}

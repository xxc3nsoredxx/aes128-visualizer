vpath %.c src
vpath %.h src
vpath %.o obj

OBJS = aesvars.o main.o ops.o output_ctrl.o
CC = gcc
CFLAGS = -Wall -Wextra -c
LFLAGS = $(shell pkg-config --libs panel) $(shell pkg-config --libs ncurses)

.PHONY: all
all: aes128-vis

aes128-vis: $(OBJS)
	$(CC) $^ $(LFLAGS) -o $@

obj/aesvars.o: aesvars.c aesvars.h
	$(CC) $(CFLAGS) $< -o $@

obj/main.o: main.c aesvars.h ops.h output_ctrl.h
	$(CC) $(CFLAGS) $< -o $@

obj/ops.o: ops.c aesvars.h ops.h output_ctrl.h
	$(CC) $(CFLAGS) $< -o $@

obj/output_ctrl.o: output_ctrl.c aesvars.h output_ctrl.h
	$(CC) $(CFLAGS) $< -o $@

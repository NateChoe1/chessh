SRC=$(wildcard *.c)
OUT=$(subst .c,.exe,$(SRC))

CFLAGS=-O2 -ggdb

all: $(OUT)

%.exe: %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: all

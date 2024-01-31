SRC=$(wildcard *.c)
OUT=$(subst .c,,$(SRC))

CFLAGS=-O2 -ggdb

all: $(OUT)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: all

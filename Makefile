SRC_SHARED = $(wildcard src/shared/*.c)
OBJ_SHARED = $(subst .c,.o,$(subst src,work,$(SRC_SHARED)))
SRC_DAEMON = $(wildcard src/daemon/*.c)
OBJ_DAEMON = $(subst .c,.o,$(subst src,work,$(SRC_DAEMON)))
SRC_CLIENT = $(wildcard src/client/*.c)
OBJ_CLIENT = $(subst .c,.o,$(subst src,work,$(SRC_CLIENT)))

HEADERS_SHARED = $(wildcard src/include/*.h)
HEADERS_DAEMON = $(wildcard src/include/daemon/*.h)
HEADERS_CLIENT = $(wildcard src/include/client/*.h)

LDFLAGS_SHARED =
LDFLAGS_DAEMON =
LDFLAGS_CLIENT =

CFLAGS_SHARED := -O2 -pipe -Wall -Wpedantic -Wshadow -ansi
CFLAGS_DAEMON :=
CFLAGS_CLIENT :=

CFLAGS_SHARED += -Isrc/include
INSTALLDIR := /usr/bin/

OUT_CLIENT = chessh-client
OUT_DAEMON = chessh-daemon

all: build/$(OUT_DAEMON) build/$(OUT_CLIENT)

build/$(OUT_DAEMON): $(OBJ_SHARED) $(OBJ_DAEMON)
	$(CC) $(OBJ_SHARED) $(OBJ_DAEMON) $(LDFLAGS_SHARED) $(LDFLAGS_DAEMON) -o build/$(OUT_DAEMON)

build/$(OUT_CLIENT): $(OBJ_SHARED) $(OBJ_CLIENT)
	$(CC) $(OBJ_SHARED) $(OBJ_CLIENT) $(LDFLAGS_SHARED) $(LDFLAGS_CLIENT) -o build/$(OUT_CLIENT)

work/shared/%.o: src/shared/%.c $(HEADERS_SHARED)
	$(CC) -c $(CFLAGS_SHARED) $< -o $@

work/daemon/%.o: src/daemon/%.c $(HEADERS_SHARED) $(HEADERS_DAEMON)
	$(CC) -c $(CFLAGS_SHARED) $(CFLAGS_DAEMON) $< -o $@

work/client/%.o: src/client/%.c $(HEADERS_SHARED) $(HEADERS_CLIENT)
	$(CC) -c $(CFLAGS_SHARED) $(CFLAGS_CLIENT) $< -o $@

install:
	cp build/$(OUT_DAEMON) $(INSTALLDIR)/$(OUT)
	cp build/$(OUT_CLIENT) $(INSTALLDIR)/$(OUT)

uninstall:
	rm $(INSTALLDIR)/$(OUT_DAEMON)
	rm $(INSTALLDIR)/$(OUT_CLIENT)

.PHONY: all install uninstall

CC = aztec43_acc
HOSTCC ?= cc
ASM86 = cpm86_asm86
GENCMD = cpm86_gencmd
EMU2 ?= emu2
HACKING ?= ../../cpm86-hacking
CFLAGS = -target=cpm86 -squeeze

all: foundry.cmd

foundry.cmd: fdplay.o foundry.o fdtick.o util.lib
	$(CC) -target=cpm86 -o $@ $^

fdplay.o: fdplay.c foundry.h conio.h
	$(CC) $(CFLAGS) -I. -c -o $@ $<

foundry.o: foundry.h

# Aztec's DOS tools can only see files inside the invocation directory.
conio.h: $(HACKING)/conio.h
	cp $< $@

util.lib: $(HACKING)/util.lib
	cp $< $@

$(HACKING)/util.lib: $(HACKING)/conio.c $(HACKING)/conio.h
	$(MAKE) -C $(HACKING) util.lib

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	$(CC) $(CFLAGS) -c -o $@ $<

tetris.cmd: tetris.h86
	$(GENCMD) $< 8080

tetris.h86: tetris.a86
	$(ASM86) $<

test: foundry.cmd
	@check=$$(mktemp); trap 'rm -f "$$check"' EXIT; \
	    $(HOSTCC) -std=c89 -Wall -Wextra -Werror -o "$$check" fdtest.c foundry.c && "$$check"
	python3 fdtest.py "$(EMU2)"

clean:
	rm -f conio.h util.lib foundry.cmd fdplay.o foundry.o fdtick.o tetris.cmd tetris.h86 tetris.lst tetris.sym

.DELETE_ON_ERROR:
.PHONY: all test clean

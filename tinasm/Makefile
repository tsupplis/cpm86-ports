# Makefile for tinasm, the NASM-compatible 8086 assembler, built for CP/M-86
# with Aztec C (aztec43_acc) from the cpm86-crossdev toolchain.
#
#   make          builds tinasm.cmd
#   make clean    removes the build outputs
#
# Only tinasm.c is compiled.
#
# The instruction table does not fit in the string space of the Aztec
# compiler, so it lives outside the program: instr.txt -> mkinstr.py ->
# instr.bin, which cmdmod adds to tinasm.cmd as group 2 (type EXTRA), and
# tinasm reads it at run time with the cmdseg API.
#
# Draft: the flags come from the template Makefile and are not verified yet.

CC = aztec43_acc
LD = aztec43_acc
CMDINFO = pcdev_cmdinfo
CMDMOD = pcdev_cmdmod
PYTHON = python3
CFLAGS = -I. -squeeze
LDFLAGS = -target=cpm86 -L. -lutil

OBJS = tinasm.o

.PHONY: all clean test

all: tinasm.cmd

tinasm.o: tinasm.c cmdseg.h
	$(CC) $(CFLAGS) -c -o $@ $<

instr.bin: instr.txt mkinstr.py
	$(PYTHON) mkinstr.py instr.txt $@

tinasm.cmd: $(OBJS) instr.bin
	$(LD) -o $@ $(OBJS) $(LDFLAGS)
	$(CMDMOD) -s instr.bin -t EXTRA $@ 2
	$(CMDINFO) $@

# Assemble the programs in test/ under emu2 and compare against the
# reference binaries. Add a line here as each program starts to work.
test: tinasm.cmd
	@sh runtest.sh input0 bin input0.img >&2
	@sh runtest.sh input1 bin input1.img >&2
	@sh runtest.sh input2 bin input2.img >&2
	@sh runtest.sh include cmd include.img >&2
	@sh runtest.sh hello cmd hello.img >&2
	@sh runtest.sh basic bin basic.img >&2
	@sh runtest.sh os bin os.img >&2
	@sh runtest.sh bricks bin bricks.img >&2
	@sh runtest.sh fbird bin fbird.img >&2
	@sh runtest.sh invaders bin invaders.img >&2
	@sh runtest.sh pillman bin pillman.img >&2
	@sh runtest.sh rogue bin rogue.img >&2

clean:
	rm -f *.o *.cmd instr.bin test/*.out test/*.log

# edlin: PC-DOS 1.1 EDLIN ported to CP/M-86.
# The original DOS source (a MASM reconstruction of PC-DOS 1.1 EDLIN.COM) is in
# https://github.com/tsupplis/pcdos11-hacking

GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
EXE2BIN=pcdev_exe2bin
CMDINFO=cmdinfo
CMDINFO=cmdinfo
MASM=pcdev_masm
LINK=pcdev_link

BINARIES=edlin.cmd

all: $(BINARIES)

edlin.cmd: edlin.exe
	$(EXE2BIN) $< edlin.bin
	$(BIN2CMD) -m 10000 edlin.bin $@
	$(CMDINFO) $@

edlin.exe: edlin.obj
	$(LINK) $< \;

edlin.obj: edlin.asm
	unix2dos $<
	$(MASM) $(basename $@),$(basename $@)\;
	[ -f $@ ]

test: all
	python3 test.py

run: edlin.cmd
	emu2 $< $(FILE)

clean:
	rm -rf *.h86 *.lst *.sys *.cmd *.hex *.sym *.log *.bin *.exe *.com

# Use the repository's configured PCE images and launcher for manual checks.
final-test: all
	$(MAKE) -C .. final-test

.PHONY: all clean test run final-test

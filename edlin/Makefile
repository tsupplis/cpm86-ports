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

run: edlin.cmd
	emu2 $< $(FILE)

clean:
	rm -rf *.h86 *.lst *.sys *.cmd *.hex *.sym *.log *.bin *.exe *.com

cpmtest.img: binaries
	cpmrm -f ibmpc-514ss cpmtest.img 0:*.*
	cpmcp -f ibmpc-514ss cpmtest.img $(BINARIES) 0:
	cpmls -F -f ibmpc-514ss cpmtest.img 0:*.*

test: cpmtest.img
	./cpm86

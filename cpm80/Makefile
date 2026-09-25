LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
CMDINFO=cmdinfo

BINARIES=vcpm.cmd 80x86.cmd z80.cmd

all: $(BINARIES)

vcpm.cmd: vcpm.h86
	$(GENCMD) $< 'extra[M1000]'
	$(CMDINFO) $@ 

80x86.cmd: 80x86.h86
	$(GENCMD) $< 'extra[M1000]'
	$(CMDINFO) $@ 

z80.cmd: z80.h86
	$(GENCMD) $< 'extra[MC00]'
	$(CMDINFO) $@ 

%.obj: %.a86
	$(RASM86) $<

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -rf *.h86 *.lst *.sys *.cmd *.hex *.sym *.log

cpmtest.img: binaries
	cpmrm -f ibmpc-514ss cpmtest.img 0:*.*
	cpmcp -f ibmpc-514ss cpmtest.img $(BINARIES) 0:
	cpmcp -f ibmpc-514ss cpmtest.img samples/ver.* 0:
	cpmcp -f ibmpc-514ss cpmtest.img samples/turbo.* 0:
	cpmls -F -f ibmpc-514ss cpmtest.img 0:*.*

test: cpmtest.img
	./cpm86

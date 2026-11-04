LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
CMDINFO=cmdinfo

BINARIES=vcpm.cmd 80x86.cmd z80.cmd cpuid.cmd
VCPM_MODULES=vcpinit.inc vcptrap.inc vcpname.inc vcpdata.inc

all: $(BINARIES)

cpuid.cmd: cpuid.h86
	$(GENCMD) $< 
	$(CMDINFO) $@

vcpm.cmd: vcpm.h86
	$(GENCMD) $< 'extra[M1000]'
	$(CMDINFO) $@ 

80x86.cmd: 80x86.h86
	$(GENCMD) $< 'extra[M1000]'
	$(CMDINFO) $@ 

z80.cmd: z80.h86
	$(GENCMD) $< 'extra[MC00]'
	$(CMDINFO) $@ 

vcpm.h86: vcpm.a86 $(VCPM_MODULES)

z80.h86: z80.a86 jumpaddr.z80 edaddr.z80 cbaddr.z80 code8086.z80 codecb.z80 codeed.z80 codexy.z80

%.obj: %.a86
	$(RASM86) $<

%.h86: %.a86
	$(ASM86) $<

test: z80.cmd
	python3 test.py

.PHONY: all clean test

clean:
	rm -rf *.h86 *.lst *.sys *.cmd *.hex *.sym *.log

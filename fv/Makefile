ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo

all: fv.cmd

fv.cmd: fv.h86
	$(GENCMD) $<
	$(CMDINFO) $@

%.h86: %.a86
	unix2dos $<
	$(ASM86) $<

clean:
	rm -rf *.h86 *.lst *.sym *.h86 *.cmd *.log *.bin
	rm -rf *.xxd *.map


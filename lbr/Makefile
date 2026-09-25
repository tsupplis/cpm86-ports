ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
CMDINFO=cmdinfo

BINARIES=lbrdir.cmd lbrtype.cmd


all: $(BINARIES) 

lbrtype.cmd: lbrtype.h86
	#from code
	$(GENCMD) $< 'DATA[M200]'
	$(CMDINFO) $@

lbrdir.cmd: lbrdir.h86
	$(GENCMD) $< 8080 
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -rf *.h86 *.lst *.sym *.h86 *.cmd *.cmd *.log *.bin
	rm -rf *.xxd *.map
	rm -rf cpmtest?.img


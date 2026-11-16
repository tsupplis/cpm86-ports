ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: frag.cmd

frag.cmd: frag.h86
	$(GENCMD) $< 'DATA[M280]'
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

test: all
	python3 test.py

.DELETE_ON_ERROR:
.PHONY: all clean test

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: du.cmd

du.cmd: du.h86
	$(GENCMD) $< 8080 'CODE[MF00]'
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

test: all
	python3 test.py

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

.DELETE_ON_ERROR:
.PHONY: all clean test

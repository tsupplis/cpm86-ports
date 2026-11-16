ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: page.cmd

page.cmd: page.h86
	$(GENCMD) $<
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

test: all
	python3 test.py

.DELETE_ON_ERROR:
.PHONY: all clean test

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: patcher.cmd

patcher.cmd: patcher.h86
	$(GENCMD) $< 8080 'CODE[M200]'
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

test: all
	python3 test.py

.DELETE_ON_ERROR:
.PHONY: all clean test

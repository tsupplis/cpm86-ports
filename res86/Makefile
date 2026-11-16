ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: res86.cmd

res86.cmd: res86.h86
	$(GENCMD) $< 'DATA[MFF0]'
	$(CMDINFO) $@

res86.h86: res86.a86 res86b.a86
	$(ASM86) res86.a86

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

test: all
	python3 test.py

.DELETE_ON_ERROR:
.PHONY: all clean test

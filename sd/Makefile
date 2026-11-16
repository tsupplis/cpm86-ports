ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: sd.cmd

sd.cmd: sd.h86
	$(GENCMD) $< 8080 'CODE[M3FF]'
	$(CMDINFO) $@

sd.h86: sd.a86
	$(ASM86) $<

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

test: all
	python3 test.py
	python3 testbdos.py

.DELETE_ON_ERROR:
.PHONY: all clean test

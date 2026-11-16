ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: modem9.cmd

modem9.cmd: modem9.h86
	$(GENCMD) $< 8080
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -f *.h86 *.lst *.sym *.cmd *.log

test:

.DELETE_ON_ERROR:
.PHONY: all clean test

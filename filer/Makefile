ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo

all: filer.cmd

filer.cmd: filer.h86
	$(GENCMD) $< 8080 'CODE[M3FF,XF00]'
	$(CMDINFO) $@

%.h86: %.a86
	unix2dos $<
	$(ASM86) $<

clean:
	rm -rf *.h86 *.lst *.sym *.h86 *.cmd *.log *.bin
	rm -rf *.xxd *.map


.PHONY: test-keys
test-keys:
	python3 testkeys.py

.PHONY: test-exec
test-exec:
	python3 testexec.py

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo

all: seemem.cmd

seemem.cmd: seemem.h86
	$(GENCMD) $<
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

.PHONY: test-startup
test-startup:
	python3 teststartup.py

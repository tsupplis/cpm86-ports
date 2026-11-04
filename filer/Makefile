ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo
MODULES=fstart.inc fexec.inc fnav.inc ffile.inc fcopy.inc fio.inc \
	fscreen.inc fdata.inc

all: filer.cmd

filer.cmd: filer.h86
	$(GENCMD) $< 8080 'CODE[M3FF,XF00]'
	$(CMDINFO) $@

filer.h86: filer.a86 $(MODULES)

%.h86: %.a86
	unix2dos $<
	$(ASM86) $<

clean:
	rm -rf *.h86 *.lst *.sym *.h86 *.cmd *.log *.bin
	rm -rf *.xxd *.map


.PHONY: all clean test test-keys test-exec test-attrs
test: filer.cmd test-keys test-exec test-attrs

test-keys:
	python3 testkeys.py

.PHONY: test-exec
test-exec:
	python3 testexec.py

.PHONY: test-attrs
test-attrs:
	python3 testattrs.py

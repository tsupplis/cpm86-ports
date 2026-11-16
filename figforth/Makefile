ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=pcdev_cmdinfo

all: forth.cmd examples/demo.blk

forth.cmd: forth.h86
	$(GENCMD) $< 8080 'CODE[M1000,X1000]'
	$(CMDINFO) $@

forth.h86: forth.a86 cpmio.a86
	$(ASM86) forth

examples/demo.blk: examples/mkblocks.py $(wildcard examples/*.fth)
	python3 examples/mkblocks.py

test: all
	python3 tests/test.py

clean:
	rm -f forth.cmd forth.h86 forth.lst forth.sym examples/demo.blk

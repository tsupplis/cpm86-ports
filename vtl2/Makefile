ASM86=cpm_asm86
ASM80=cpm_asm80
GENCMD=cpm_gencmd
HEXCOM=hexcom
XLT86=cpm_xlt86

TARGET=vtl2

all: vtl2.cmd vtl2.com

vtl2.h86: vtl2.a86
	unix2dos vtl2.asm
	$(ASM86) $(basename $<)

vtl2.com: vtl2.hex
	$(HEXCOM) $<

vtl2.cmd: vtl2.h86
	$(GENCMD) $< 8080

vtl2.hex: vtl2.asm
	unix2dos vtl2.asm
	$(ASM80) $(basename $<)

vtl2.a86: vtl2.asm
	$(XLT86) $(basename $<)

test:
	sh test.sh

clean:
	rm -f vtl2.com vtl2.cmd vtl2.hex vtl2.h86 vtl2.prn \
        vtl2.lst vtl2.sym vtl2.obj vtl2.log vtl2.a86

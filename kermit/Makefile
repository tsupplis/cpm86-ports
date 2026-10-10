ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

all: kermibm.cmd

kermibm.cmd: kermibm.h86
	$(GENCMD) $<

kermibm.h86: c86ker.a86 c86cmd.a86 c86fil.a86 c86xibm.a86 c86pro.a86 c86trm.a86 c86utl.a86
	$(ASM86) c86ker
	mv c86ker.h86 $@

clean:
	rm -f kermibm.h86 kermibm.cmd c86ker.prn c86ker.sym c86ker.lst

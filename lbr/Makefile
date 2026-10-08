AS=aztec42_as
CC=aztec42_cc
AR=aztec42_lib
CFLAGS=-I. +F -B +0 -DAZTEC
STRIP=aztec42_sqz
LDFLAGS=-lm -lc86
LD=aztec42_link
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
CMDINFO=cmdinfo

BINARIES=lbrdir.cmd lbrtype.cmd delbr.cmd lu86.cmd


all: $(BINARIES) 

lu86.cmd: lu86.o
	$(LD) -o $@ lu86.o $(LDFLAGS)

delbr.cmd: delbr.o
	$(LD) -o $@ delbr.o $(LDFLAGS)

lbrtype.cmd: lbrtype.h86
	#from code
	$(GENCMD) $< 'DATA[M200]'
	$(CMDINFO) $@

lbrdir.cmd: lbrdir.h86
	$(GENCMD) $< 8080 
	$(CMDINFO) $@

%.h86: %.a86
	$(ASM86) $<

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@


clean:
	rm -rf *.h86 *.lst *.sym *.h86 *.cmd *.cmd *.log *.bin
	rm -rf *.xxd *.map *.o
	rm -rf cpmtest?.img


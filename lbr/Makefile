CC=aztec43_acc
AR=aztec42_lib
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86 -lm
LD=aztec43_acc
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
BIN2CMD=bin2cmd
CMDINFO=cmdinfo

BINARIES=lbrdir.cmd lbrtype.cmd delbr.cmd lu86.cmd

all: $(BINARIES) 

lu86.cmd: lu86.o
	$(LD) $(LDFLAGS) -o $@ lu86.o

delbr.cmd: delbr.o
	$(LD) $(LDFLAGS) -o $@ delbr.o

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
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.h86 *.lst *.sym *.h86 *.cmd *.cmd *.log *.bin
	rm -rf *.xxd *.map *.o
	rm -rf cpmtest?.img

test: all
	python3 test.py

.PHONY: all test clean

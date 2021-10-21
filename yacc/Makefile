AS=aztec34_as
CC=aztec34_cc
AR=aztec34_lib
CFLAGS=-I. -B +0 -D__CPM86__ 
STRIP=aztec34_sqz
LDFLAGS=-lc86
LD=aztec34_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86

BINARIES=yacc.cmd

all: binaries

binaries: $(BINARIES) 

yacc.cmd: y1.o y2.o y3.o y4.o
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES)
	$(RM) cpmtest.img

cpmtest.img: binaries Makefile sample.y yaccpar.tpl
	(cd pce;make binaries)
	cp cpmbase.img cpmtest.img
	cpmcp -f ibmpc-514ss cpmtest.img $(BINARIES) 0:
	cpmcp -f ibmpc-514ss cpmtest.img sample.y 0:
	cpmcp -f ibmpc-514ss cpmtest.img yaccpar.tpl 0:
	cpmls -F -f ibmpc-514ss cpmtest.img 0:*.*

test: cpmtest

cpmtest: cpmtest.img
	@./cpm86

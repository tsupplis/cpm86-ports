AS=aztec42_as
ACC=aztec42_cc
AR=aztec42_lib
CFLAGS=-I. -B +0 -D__CPM86__ 
STRIP=aztec42_sqz
LDFLAGS=-lc86
LD=aztec42_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86

BINARIES=ansi2kr.cmd

all: binaries ansi2kr

binaries: $(BINARIES) 

ansi2kr: ansi2krn.o
	$(CC) -O2 -o $@ $<

ansi2krn.o: ansi2kr.c
	$(CC) -O2 -c -o $@ $<

ansi2kr.cmd: ansi2kr.o
	$(LD) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(ACC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES)
	$(RM) cpmtest.img

cpmtest.img: binaries Makefile orig.c
	(cd pce;make binaries)
	cp cpmbase.img cpmtest.img
	cpmcp -f ibmpc-514ss cpmtest.img $(BINARIES) 0:
	cpmcp -f ibmpc-514ss cpmtest.img orig.c 0:
	cpmls -F -f ibmpc-514ss cpmtest.img 0:*.*

test: cpmtest

cpmtest: cpmtest.img
	@./cpm86

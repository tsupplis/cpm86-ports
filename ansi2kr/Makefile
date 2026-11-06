CC=aztec43_acc
LD=aztec43_acc
AR=aztec42_lib
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86
HOST_CC=cc
HOST_CFLAGS=-Wno-deprecated-non-prototype

BINARIES=ansi2kr.cmd

all: binaries ansi2kr

binaries: $(BINARIES) 

ansi2kr: ansi2krn.o
	$(HOST_CC) $(HOST_CFLAGS) -O2 -o $@ $<

ansi2krn.o: ansi2kr.c
	$(HOST_CC) $(HOST_CFLAGS) -O2 -c -o $@ $<

ansi2kr.cmd: ansi2kr.o
	$(LD) $(LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES) ansi2kr
	$(RM) cpmtest.img

cpmtest.img: binaries Makefile orig.c
	cp cpmbase.img cpmtest.img
	cpmcp -f ibmpc-514ss cpmtest.img $(BINARIES) 0:
	cpmcp -f ibmpc-514ss cpmtest.img orig.c 0:
	cpmls -F -f ibmpc-514ss cpmtest.img 0:*.*

# Interactive PCE validation; excluded from automated make test.
final-test: cpmtest.img
	@./cpm86

test: all
	python3 test.py

.PHONY: all binaries clean final-test test

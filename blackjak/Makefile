CC=aztec43_acc
LD=aztec43_acc
HOSTCC ?= cc
EMU2 ?= emu2
CFLAGS=-squeeze
CPM86_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

binaries: blackjak.cmd blackjak.com

blackjak.com: blackjak.o rules.o
	$(LD) -o $@ $^ $(DOS_LDFLAGS)

blackjak.cmd: bjmaincp.o rulescp.o
	$(LD) -o $@ $^ $(CPM86_LDFLAGS) 

test: binaries
	python3 test.py "$(HOSTCC)" "$(EMU2)"

bjmaincp.o: blackjak.c rules.h
	$(CC) -target=cpm86 -DCPM_VT52 $(CFLAGS) -c -o $@ $<

blackjak.o: blackjak.c rules.h
	$(CC) -target=dos1 $(CFLAGS) -c -o $@ $<

rulescp.o: rules.c rules.h
	$(CC) -target=cpm86 $(CFLAGS) -c -o $@ $<

rules.o: rules.c rules.h
	$(CC) -target=dos1 $(CFLAGS) -c -o $@ $<

clean:
	rm -f blackjak.o bjmaincp.o rules.o rulescp.o blackjak.cmd blackjak.com

.DELETE_ON_ERROR:
.PHONY: all binaries test clean

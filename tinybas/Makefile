CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

CMDS=tinybas.com tinybas.cmd

binaries: $(CMDS)

tinybas.o: tinybas.c
	$(CC) -c $< $(CFLAGS)

tinybas.com:tinybas.o
	$(LD) -o $@ $< $(DOS_LDFLAGS)

tinybas.cmd:tinybas.o
	$(LD) -o $@ $< $(CPM_LDFLAGS)

clean:
	rm -rf *.o *.com *.cmd


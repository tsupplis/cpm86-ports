CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lm -lc86 
DOS_LDFLAGS=-lm -lc

all: binaries

binaries: trek.cmd trek.com

trek.com: trek.o
	$(LD) -o $@ trek.o $(DOS_LDFLAGS)

trek.cmd: trekcp.o
	$(LD) -o $@ trekcp.o $(CPM_LDFLAGS)

test: binaries
	sh test.sh

trekcp.o: trek.c
	$(CC) -D__CPM86__ $(CFLAGS) -c -o $@ $<

trek.o: trek.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o *.cmd *.com

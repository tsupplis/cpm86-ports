CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

binaries: uuencode.cmd uudecode.cmd uuencode.com uudecode.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

uuencode.cmd: uuencode.o
	$(LD) -o $@ uuencode.o $(CPM_LDFLAGS)

uudecode.cmd: uudecode.o
	$(LD) -o $@ uudecode.o $(CPM_LDFLAGS)

uuencode.com: uuencode.o
	$(LD) -o $@ uuencode.o $(DOS_LDFLAGS)

uudecode.com: uudecode.o
	$(LD) -o $@ uudecode.o $(DOS_LDFLAGS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

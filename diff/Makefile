CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=diff.o diffreg.o

binaries: diff.cmd diff.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

diff.o: diff.c diff.h
diffreg.o: diffreg.c diff.h

diff.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

diff.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

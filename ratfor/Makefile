CC=aztec42_cc
LD=aztec42_link
CFLAGS=-DHAVE_CONFIG_H
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=ratfor.o lookup.o getopt.o

binaries: ratfor.cmd ratfor.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

ratfor.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

ratfor.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

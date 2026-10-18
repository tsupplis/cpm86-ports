CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=tr.o

binaries: tr.cmd tr.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tr.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

tr.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

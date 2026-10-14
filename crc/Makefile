CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=crc.o

binaries: crc.cmd crc.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

crc.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

crc.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

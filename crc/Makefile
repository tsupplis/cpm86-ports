CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-I. -squeeze
CPM86_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=crc.o

binaries: crc.cmd crc.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

crc.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

crc.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)
	$(CMDINFO) $@

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

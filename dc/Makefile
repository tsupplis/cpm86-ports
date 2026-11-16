CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-I. -squeeze
CPM86_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos2

all: binaries

OBJS=dc.o

binaries: dc.cmd dc.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

dc.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

dc.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)
	$(CMDINFO) $@

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

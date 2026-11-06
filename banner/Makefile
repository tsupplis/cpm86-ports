CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=banner.o

binaries: banner.cmd banner.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

banner.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

banner.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)

clean:
	rm -rf *.o *.cmd *.com

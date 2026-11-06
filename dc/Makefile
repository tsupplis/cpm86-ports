CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos2

all: binaries

OBJS=dc.o

binaries: dc.cmd dc.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

dc.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

dc.cmd: $(OBJS)
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

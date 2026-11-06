CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze -DHAVE_CONFIG_H
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=ratfor.o lookup.o getopt.o

binaries: ratfor.cmd ratfor.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

ratfor.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

ratfor.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

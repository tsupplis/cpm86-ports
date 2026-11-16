CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-I. -squeeze
CPM86_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=diff.o diffreg.o

binaries: diff.cmd diff.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

diff.o: diff.c diff.h
diffreg.o: diffreg.c diff.h

diff.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

diff.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)
	$(CMDINFO) $@

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py diff

clean:
	rm -rf *.o *.cmd *.com

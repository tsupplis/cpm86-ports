CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-squeeze
CPM86_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=sort.o

binaries: sort.cmd sort.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sort.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

sort.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)
	$(CMDINFO) $@

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py sort

clean:
	rm -rf *.o *.cmd *.com

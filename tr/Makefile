CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-squeeze
CPM86_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=tr.o

binaries: tr.cmd tr.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tr.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

tr.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)
	$(CMDINFO) $@

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py tr

clean:
	rm -rf *.o *.cmd *.com

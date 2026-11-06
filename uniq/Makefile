CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=uniq.o

binaries: uniq.cmd uniq.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

uniq.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

uniq.cmd: $(OBJS)
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJS)

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py uniq

clean:
	rm -rf *.o *.cmd *.com

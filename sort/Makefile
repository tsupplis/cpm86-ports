CC=aztec42_acc
LD=aztec42_acc
CFLAGS=-squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=sort.o

binaries: sort.cmd sort.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sort.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

sort.cmd: $(OBJS)
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJS)

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py sort

clean:
	rm -rf *.o *.cmd *.com

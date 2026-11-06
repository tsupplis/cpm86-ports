CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=grep.o

binaries: grep.cmd grep.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

grep.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

grep.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py grep

clean:
	rm -rf *.o *.cmd *.com

CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=sort.o

binaries: sort.cmd sort.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sort.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

sort.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py sort

clean:
	rm -rf *.o *.cmd *.com

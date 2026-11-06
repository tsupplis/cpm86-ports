CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJS=sed0.o sed1.o

binaries: sed.cmd sed.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sed0.o: sed0.c sed.h
sed1.o: sed1.c sed.h

sed.com: $(OBJS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJS)

sed.cmd: $(OBJS)
	$(LD) $(CPM86_LDFLAGS) -o $@ $(OBJS)

test: binaries
	sh test.sh
	python3 ../tools/test-consistency.py sed

clean:
	rm -rf *.o *.cmd *.com

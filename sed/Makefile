CC=aztec42_cc
LD=aztec42_link
CFLAGS=
CPM_LDFLAGS=-lc86
DOS_LDFLAGS=-lc

all: binaries

OBJS=sed0.o sed1.o

binaries: sed.cmd sed.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sed0.o: sed0.c sed.h
sed1.o: sed1.c sed.h

sed.com: $(OBJS)
	$(LD) -o $@ $(OBJS) $(DOS_LDFLAGS)

sed.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(CPM_LDFLAGS)

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

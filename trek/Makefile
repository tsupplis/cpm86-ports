CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-squeeze
CPM86_LDFLAGS=-target=cpm86 -lm
DOS_LDFLAGS=-target=dos1 -lm

all: binaries

binaries: trek.cmd trek.com

trek.com: trek.o
	$(LD) -o $@ trek.o $(DOS_LDFLAGS)

trek.cmd: trekcp.o
	$(LD) -o $@ trekcp.o $(CPM86_LDFLAGS) 

test: binaries
	sh test.sh

trekcp.o: trek.c
	$(CC) -target=cpm86 $(CFLAGS) -c -o $@ $<

trek.o: trek.c
	$(CC) -target=dos1 $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o *.cmd *.com

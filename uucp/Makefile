CC=aztec42_acc
LD=aztec42_acc
CFLAGS=-squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

binaries: uuencode.cmd uudecode.cmd uuencode.com uudecode.com

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

uuencode.cmd: uuencode.o
	$(LD) $(CPM_LDFLAGS) -o $@ uuencode.o

uudecode.cmd: uudecode.o
	$(LD) $(CPM_LDFLAGS) -o $@ uudecode.o

uuencode.com: uuencode.o
	$(LD) $(DOS_LDFLAGS) -o $@ uuencode.o

uudecode.com: uudecode.o
	$(LD) $(DOS_LDFLAGS) -o $@ uudecode.o

test: binaries
	sh test.sh

clean:
	rm -rf *.o *.cmd *.com

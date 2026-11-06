CC=aztec42_acc
LD=aztec42_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86

all: kilo.cmd

kilo.cmd: kilo.o
	$(LD) $(CPM_LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.cmd *.exe

.PHONY: all clean

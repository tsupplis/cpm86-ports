CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86

all: advent.cmd advent.dat

OBJS=main.o wizard.o save.o subr.o vocab.o init.o io.o done.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# util.lib and its headers are local snapshots supplied from cpm86-hacking.
advent.cmd: $(OBJS) 
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJS)
# The data file holds the game structures, so it must be built by this
# binary (the CCP upcases the argument; main.c compares ignoring case).
advent.dat: advent.cmd advent.msg
	emu2 advent.cmd advent.msg

test: advent.cmd advent.dat

clean:
	rm -rf *.o *.cmd

.PHONY: test-keys
test-keys:
	python3 testkeys.py

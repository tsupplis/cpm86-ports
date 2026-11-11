CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86

all: advent.cmd

OBJS=main.o wizard.o save.o subr.o vocab.o init.o io.o done.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# util.lib and its headers are local snapshots supplied from cpm86-hacking.
advent.cmd: $(OBJS) 
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJS)
test: advent.cmd

clean:
	rm -rf *.o *.cmd

.PHONY: test-keys
test-keys:
	python3 testkeys.py

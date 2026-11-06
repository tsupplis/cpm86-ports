CC=aztec42_acc
LD=aztec42_acc
CFLAGS=-I. -DAZTEC -squeeze
CPM_LDFLAGS=-target=cpm86

all: sokoban.cmd

OBJS=sokoban.o gameio.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# util.lib and its headers are local snapshots supplied from cpm86-hacking.
sokoban.cmd: $(OBJS) util.lib
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJS) util.lib

sokoban.o: config.h gameio.h conio.h
gameio.o: gameio.h conio.h util.h

test: sokoban.cmd

clean:
	rm -rf *.o *.cmd

.PHONY: test-keys
test-keys:
	python3 testkeys.py

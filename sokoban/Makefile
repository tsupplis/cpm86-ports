CC=aztec42_cc
LD=aztec42_link
CFLAGS=-I. -DAZTEC
CPM_LDFLAGS=-lc86

all: sokoban.cmd

OBJS=sokoban.o gameio.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# util.lib and its headers are local snapshots supplied from cpm86-hacking.
sokoban.cmd: $(OBJS) util.lib
	$(LD) -o $@ $(OBJS) util.lib $(CPM_LDFLAGS)

sokoban.o: config.h gameio.h conio.h
gameio.o: gameio.h conio.h util.h

test: sokoban.cmd

clean:
	rm -rf *.o *.cmd

.PHONY: test-keys
test-keys:
	python3 testkeys.py

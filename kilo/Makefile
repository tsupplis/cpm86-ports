CC=aztec42_cc
LD=aztec42_link
CPM_LDFLAGS=-lc86

all: kilo.cmd

kilo.cmd: kilo.o
	$(LD) -o $@ $< $(CPM_LDFLAGS)

%.o: %.c
	$(CC) $<

clean:
	rm -f *.o *.cmd *.exe

.PHONY: all clean

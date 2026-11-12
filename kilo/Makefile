CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-I. -squeeze
CPM_LDFLAGS=-target=cpm86

all: kilo.cmd

kilo.cmd: kilo.o
	$(LD) $(CPM_LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.cmd *.exe
	rm -rf tests

tests/testfile.cmd: testfile.c kilo.c fsave.h
	mkdir -p tests
	$(CC) $(CFLAGS) $(CPM_LDFLAGS) -o $@ testfile.c

test: kilo.cmd tests/testfile.cmd
	python3 test-file.py

.PHONY: all clean test

# Shared portable file replacement implementation.
kilo.o: ../util/fsave.h

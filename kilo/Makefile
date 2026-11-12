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

tests/testsave.cmd: testsave.c fsave.h
	mkdir -p tests
	$(CC) $(CFLAGS) $(CPM_LDFLAGS) -o $@ testsave.c

test: kilo.cmd tests/testfile.cmd tests/testsave.cmd
	python3 test-file.py
	python3 test-save.py

.PHONY: all clean test

# Local file replacement implementation.
kilo.o: fsave.h

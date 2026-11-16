CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo

CFLAGS=-I. -squeeze
CPM86_LDFLAGS=-target=cpm86

all: kilo.cmd

kilo.cmd: kilo.o
	$(LD) $(CPM86_LDFLAGS) -o $@ $<
	$(CMDINFO) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.cmd *.exe
	rm -rf tests

tests/testfile.cmd: testfile.c kilo.c fsave.h
	mkdir -p tests
	$(CC) $(CFLAGS) $(CPM86_LDFLAGS) -o $@ testfile.c

tests/testsave.cmd: testsave.c fsave.h
	mkdir -p tests
	$(CC) $(CFLAGS) $(CPM86_LDFLAGS) -o $@ testsave.c

test: kilo.cmd tests/testfile.cmd tests/testsave.cmd
	python3 test-file.py
	python3 test-save.py

.PHONY: all clean test

# Local file replacement implementation.
kilo.o: fsave.h

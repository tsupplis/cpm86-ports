CC=aztec42_acc
LD=aztec42_acc
AR=aztec42_lib
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86 -lm

all: sq.cmd usq.cmd sqtype.cmd

sq.cmd: xsq.o
	$(LD) $(LDFLAGS) -o $@ xsq.o

usq.cmd: xusq.o
	$(LD) $(LDFLAGS) -o $@ xusq.o

sqtype.cmd: xtype.o
	$(LD) $(LDFLAGS) -o $@ xtype.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj *.cmd

test: all
	python3 test.py
	python3 test-io.py

.PHONY: all test clean

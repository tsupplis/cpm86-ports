CC=aztec43_acc
LD=aztec43_acc
AR=aztec42_lib
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86 -lm

all: xlisp.cmd

OBJS=xlisp.o xllist.o xlmath.o xlstr.o xlread.o xlsubr.o xlprin.o \
    xlbind.o xldmem.o xleval.o xlfio.o xlio.o xlkmap.o xlobj.o

xlisp.cmd: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj *.cmd

test: all
	python3 test.py

.PHONY: all test clean

CC=aztec43_acc
LD=aztec43_acc
AR=aztec42_lib
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86

BINARIES=yacc.cmd

all: binaries

binaries: $(BINARIES) 

yacc.cmd: y1.o y2.o y3.o y4.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES)

.PHONY: all binaries clean

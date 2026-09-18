AS=aztec42_as
CC=aztec42_cc
AR=aztec42_lib
CFLAGS=-I. -B +0 -D__CPM86__ 
STRIP=aztec42_sqz
LDFLAGS=-lc86
LD=aztec42_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86

BINARIES=yacc.cmd

all: binaries

binaries: $(BINARIES) 

yacc.cmd: y1.o y2.o y3.o y4.o
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES)

AS=aztec42_as
CC=aztec42_cc
AR=aztec42_lib
CFLAGS=-I. +F -B +0 -DAZTEC
STRIP=aztec42_sqz
LDFLAGS=-lm -lc86
LD=aztec42_link
LINK86=pcdev_linkcmd
RASM86=pcdev_rasm86


all: sq.cmd usq.cmd sqtype.cmd

sq.cmd: xsq.o
	$(LD) -o $@ xsq.o $(LDFLAGS)

usq.cmd: xusq.o
	$(LD) -o $@ xusq.o $(LDFLAGS)

sqtype.cmd: xtype.o
	$(LD) -o $@ xtype.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $<
	$(STRIP) $@

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj *.cmd



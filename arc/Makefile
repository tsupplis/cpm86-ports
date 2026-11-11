CC=aztec43_acc
LD=aztec43_acc
CFLAGS=-target=cpm86 -I. -squeeze
LDFLAGS=-target=cpm86

BINARIES=arc.cmd marc.cmd

OBJS=arc.o arcadd.o arccode.o arccvt.o arcdata.o arcdel.o arcdos.o \
	arcext.o arcio.o arclst.o arclzw.o arcmatch.o arcpack.o arcrun.o \
	arcsq.o arcsvc.o arctst.o arcunp.o arcusq.o arcmisc.o

MOBJS=marc.o arcdata.o arcdos.o arcio.o arcmatch.o arcmisc.o

all: binaries

binaries: $(BINARIES)

arc.cmd: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

marc.cmd: $(MOBJS)
	$(LD) $(LDFLAGS) -o $@ $(MOBJS)

%.o: %.c arc.h arcs.h proto.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: binaries
	sh test.sh

clean:
	$(RM) *.o *.h86 *.log *.sym *.prn *.lst *.obj $(BINARIES)

.PHONY: all binaries test clean

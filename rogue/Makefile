CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-I. -squeeze -DCURSES -DANSI
LDFLAGS=-target=cpm86 -lm

all: rogue.cmd

OBJS=curses.o hit.o init.o invent.o level.o machdep.o main.o message.o \
	monster.o move.o object.o pack.o play.o random.o ring.o room.o \
	save.o score.o spec_hit.o throw.o trap.o use.o zap.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

rogue.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)
	$(CMDINF) $@

test: rogue.cmd

clean:
	rm -rf *.o *.cmd

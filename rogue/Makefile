CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-Osize -target=cpm86 -I. -squeeze -DCURSES -DANSI
LDFLAGS=-target=cpm86 -L. -lutil

all: rogue.cmd

OBJS=fmt.o curses.o hit.o init.o invent.o level.o machdep.o main.o message.o \
	monster.o move.o object.o pack.o play.o random.o ring.o room.o \
	save.o score.o spec_hit.o throw.o trap.o use.o zap.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

rogue.cmd: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS)
	$(CMDINFO) $@

test: rogue.cmd

clean:
	rm -rf *.o *.cmd

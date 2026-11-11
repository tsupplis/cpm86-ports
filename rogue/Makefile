CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-Osize -target=cpm86 -I. -DCURSES -DANSI
LDFLAGS=-v -target=cpm86 -L. -lutil
ROOT_OVR_FLAGS=-overlay-root=1000,400

all: rogue.cmd init.ovr save.ovr score.ovr

OBJS=fmt.o curses.o hit.o init.o invent.o level.o machdep.o main.o message.o \
	monster.o move.o object.o pack.o play.o random.o ring.o room.o \
	save.o score.o spec_hit.o throw.o trap.o use.o zap.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

rogue.cmd rogue.rsm: $(OBJS)
	$(LD) $(ROOT_OVR_FLAGS) -o $@ $(OBJS) $(LDFLAGS)
	$(CMDINFO) $@

init.ovr: initovr.o rogue.rsm
	$(LD) -target=cpm86 -overlay=rogue.rsm $< -o $@

save.ovr: saveovr.o rogue.rsm
	$(LD) -target=cpm86 -overlay=rogue.rsm $< -o $@

score.ovr: scoreovr.o rogue.rsm
	$(LD) -target=cpm86 -overlay=rogue.rsm $< -o $@

test: rogue.cmd

clean:
	rm -rf *.o *.cmd *.ovr *.rsm

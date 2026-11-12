CC=aztec43_acc
LD=aztec43_acc
CMDINFO=pcdev_cmdinfo
CFLAGS=-Osize -target=cpm86 -I. -DCURSES
TERMINAL?=VT52
ifeq ($(TERMINAL),VT52)
TERM_CFLAGS=-DVT52
else ifeq ($(TERMINAL),VT100)
TERM_CFLAGS=-DANSI
else
$(error TERMINAL must be VT52 or VT100)
endif
LDFLAGS=-v -target=cpm86 -L.
ROOT_OVR_FLAGS=-overlay-root=1000,400

all: rogue.cmd init.ovr save.ovr score.ovr

OBJS=fmt.o curses.o hit.o init.o invent.o level.o machdep.o main.o message.o \
	monster.o move.o object.o pack.o play.o random.o ring.o room.o \
	save.o score.o spec_hit.o throw.o trap.o use.o zap.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Recompile the renderer when switching terminal dialects, then relink all
# overlays against the new root symbol file.
terminal.cfg: FORCE
	@printf '%s\n' '$(TERMINAL)' > terminal.tmp
	@cmp -s terminal.tmp $@ || mv terminal.tmp $@
	@$(RM) terminal.tmp

curses.o: curses.c rogue.h terminal.cfg
	$(CC) $(CFLAGS) $(TERM_CFLAGS) -c -o $@ $<

# Also handle rapid switches on make/filesystems with coarse timestamps.
ifneq ($(shell cat terminal.cfg 2>/dev/null),$(TERMINAL))
curses.o rogue.cmd init.ovr save.ovr score.ovr: FORCE
endif

rogue.cmd rogue.rsm: $(OBJS)
	$(LD) $(ROOT_OVR_FLAGS) -o rogue.cmd $(OBJS) $(LDFLAGS)
	$(CMDINFO) rogue.cmd

init.ovr: initovr.o rogue.rsm
	$(LD) -target=cpm86 -overlay=rogue.rsm $< -o $@

save.ovr: saveovr.o rogue.rsm
	$(LD) -target=cpm86 -overlay=rogue.rsm $< -o $@

score.ovr: scoreovr.o rogue.rsm
	$(LD) -target=cpm86 -overlay=rogue.rsm $< -o $@

test: all
	ROGUE_TEST_CC="$(CC)" ROGUE_TEST_MAKE="$(MAKE)" python3 test-colors.py

clean:
	rm -rf *.o *.cmd *.ovr *.rsm
	$(RM) terminal.cfg terminal.tmp

.PHONY: all test clean FORCE

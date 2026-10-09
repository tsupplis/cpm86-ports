CC      = aztec42_cc
LD      = aztec42_link
LIB     = aztec42_lib
STRIP   = aztec42_sqz
CFLAGS  = -I. +F +0
CPM_LDFLAGS = -lc86
DOS_LDFLAGS = -lc

# emu2 (used internally by the Aztec wrappers) needs a PTY.
# Wrap every invocation that drives emu2 with expect -c spawn.
PTY = expect -c

all: ndbm.lib keydb.cmd keydb.com

# --------------------------------------------------------------------
# Library
# --------------------------------------------------------------------
ndbm.lib: ndbm.o
	rm -f ndbm.lib
	$(PTY) 'spawn $(LIB) ndbm.lib ndbm.o; expect eof'

# --------------------------------------------------------------------
# Binaries
# --------------------------------------------------------------------
keydb.cmd: keydb.o ndbm.lib
	$(PTY) 'spawn $(LD) -o $@ keydb.o ndbm.lib $(CPM_LDFLAGS); expect eof'

keydb.com: keydb.o ndbm.lib
	$(PTY) 'spawn $(LD) -o $@ keydb.o ndbm.lib $(DOS_LDFLAGS); expect eof'

# --------------------------------------------------------------------
# Compile rules
# --------------------------------------------------------------------
%.o: %.c
	$(PTY) 'spawn $(CC) $(CFLAGS) -o $@ $<; expect eof'
	$(PTY) 'spawn $(STRIP) $@; expect eof'

# --------------------------------------------------------------------
# Test
# --------------------------------------------------------------------
test: keydb.cmd keydb.com
	sh test.sh

clean:
	rm -f *.o *.lib *.cmd *.com

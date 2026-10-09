CC      = aztec42_cc
LD      = aztec42_link
LIB     = aztec42_lib
STRIP   = aztec42_sqz
CFLAGS  = -I. +F +0
CPM_LDFLAGS = -lc86
DOS_LDFLAGS = -lc

all: ndbm.lib keydb.cmd keydb.com

# --------------------------------------------------------------------
# Library
# --------------------------------------------------------------------
ndbm.lib: ndbm.o
	rm -f ndbm.lib
	$(LIB) ndbm.lib ndbm.o

# --------------------------------------------------------------------
# Binaries
# --------------------------------------------------------------------
keydb.cmd: keydb.o ndbm.lib
	$(LD) -o $@ keydb.o ndbm.lib $(CPM_LDFLAGS)

keydb.com: keydb.o ndbm.lib
	$(LD) -o $@ keydb.o ndbm.lib $(DOS_LDFLAGS)

# --------------------------------------------------------------------
# Compile rules
# --------------------------------------------------------------------
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
	$(STRIP) $@

# --------------------------------------------------------------------
# Test
# --------------------------------------------------------------------
test: keydb.cmd keydb.com
	sh test.sh

clean:
	rm -f *.o *.lib *.cmd *.com

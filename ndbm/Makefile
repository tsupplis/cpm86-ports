CC      = aztec42_acc
LD      = aztec42_acc
LIB     = aztec42_lib
CFLAGS  = -I. -squeeze
CPM_LDFLAGS = -target=cpm86
DOS_LDFLAGS = -target=dos1

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
	$(LD) $(CPM_LDFLAGS) -o $@ keydb.o ndbm.lib

keydb.com: keydb.o ndbm.lib
	$(LD) $(DOS_LDFLAGS) -o $@ keydb.o ndbm.lib

# --------------------------------------------------------------------
# Compile rules
# --------------------------------------------------------------------
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# --------------------------------------------------------------------
# Test
# --------------------------------------------------------------------
tests/testdb.cmd: testdb.o ndbm.lib
	mkdir -p tests
	$(LD) $(CPM_LDFLAGS) -o $@ testdb.o ndbm.lib

tests/testdb.com: testdb.o ndbm.lib
	mkdir -p tests
	$(LD) $(DOS_LDFLAGS) -o $@ testdb.o ndbm.lib

test: keydb.cmd keydb.com tests/testdb.cmd tests/testdb.com
	sh test.sh
	python3 test-integrity.py

clean:
	rm -f *.o *.lib *.cmd *.com
	rm -rf tests

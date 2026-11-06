CC=aztec42_acc
LD=aztec42_acc
CFLAGS=-squeeze
CPM_LDFLAGS=-target=cpm86
DOS_LDFLAGS=-target=dos1

all: binaries

OBJECTS=tinybas.o common.o errors.o express.o format.o genc.o interp.o \
	options.o parser.o statemnt.o token.o toknzr.o
CMDS=tinybas.com tinybas.cmd 
TOYS=lander.cmd lander.com wumpus.cmd wumpus.com mugwump.cmd mugwump.com \
	hurkle.cmd hurkle.com tictacto.cmd tictacto.com hammurab.cmd hammurab.com \
    mandel.cmd mandel.com

binaries: $(CMDS) 

tinybas.com: $(OBJECTS)
	$(LD) $(DOS_LDFLAGS) -o $@ $(OBJECTS)

tinybas.cmd: $(OBJECTS)
	$(LD) $(CPM_LDFLAGS) -o $@ $(OBJECTS)

%.cmd: %.o
	$(LD) $(CPM_LDFLAGS) -o $@ $<

%.com: %.o
	$(LD) $(DOS_LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
%.c: %.bas 
	$(MAKE) tinybas.com
	emu2 tinybas.com -oc $^

test: binaries $(TOYS)

clean:
	rm -rf *.o *.com *.cmd lander.c mugwump.c wumpus.c hurkle.c

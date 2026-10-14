BIN2CMD=bin2cmd
NASM=nasm

all: diskstat.cmd 

diskstat.cmd: dstat86.bin
	$(BIN2CMD) $< $@

dstat86.bin: dstat86.a86
	$(NASM) -f bin -o $@ $<

clean:
	rm -rf dstat86.bin diskstat.cmd

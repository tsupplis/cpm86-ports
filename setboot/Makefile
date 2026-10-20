BIN2CMD=pcdev_bin2cmd
CMDINFO=pcdev_cmdinfo
MASM=pcdev_masm1
LINK=pcdev_link
EXE2BIN=pcdev_exe2bin

all: setboot.com setboot.cmd

setboot.cmd: setboot.com
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

setboot.com: setboot.exe
	$(EXE2BIN) setboot.exe setboot.com

setboot.exe: setboot.obj
	$(LINK) $< \;

setboot.obj: setboot.asm
	$(MASM) $(basename $@),$(basename $@)\;
	[ -f $@ ]

clean:
	rm -rf *.obj *.bin *.com *.exe *.lst *.cmd

# Standalone personality experiment; deliberately not part of SETBOOT's all.
.PHONY: hello
hello: hello.com hello.cmd

hello.cmd: hello.com
	$(BIN2CMD) $< $@
	$(CMDINFO) $@

hello.com: hello.exe
	$(EXE2BIN) hello.exe hello.com

hello.exe: hello.obj
	$(LINK) $< \;

hello.obj: hello.asm
	$(MASM) hello,hello\;
	[ -f $@ ]

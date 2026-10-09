SUBDIRS1=filer lbr fv seemem disk7 wash15 xsq uucp ratfor

SUBDIRS2=ansi2kr tinybas xlisp yacc vtl2 dc grep sed cpu edlin sokoban diff banner

SUBDIRS=$(SUBDIRS1) $(SUBDIRS2)
all:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

clean:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done
	rm -f tmp.img cpmtest1.img

test: cpmtest1.img cpmtest2.img
	./cpm86 

cpmtest1.img:  all
	cp cpmbase.img $@
	python3 tools/cpm86twist.py untwist $@ tmp.img
	cpmls -F -f cpm86-320 tmp.img 
	for d in $(SUBDIRS1); do \
	    for i in $$d/*.cmd;do cpmcp -f cpm86-320 tmp.img $$i 0:;done; \
	done
	cpmls -F -f cpm86-320 tmp.img 
	python3 tools/cpm86twist.py twist tmp.img $@
	rm -f tmp.img


cpmtest2.img:  all
	cp cpmbase.img $@
	python3 tools/cpm86twist.py untwist $@ tmp.img
	cpmls -F -f cpm86-320 tmp.img 
	for d in $(SUBDIRS2); do \
	    for i in $$d/*.cmd;do cpmcp -f cpm86-320 tmp.img $$i 0:;done; \
	done
	for i in vtl2/*.vtl;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	for i in tinybas/*.bas;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	cpmcp -f cpm86-320 tmp.img sokoban/*.hlp 0:
	cpmcp -f cpm86-320 tmp.img sokoban/*.dat 0:
	cpmcp -f cpm86-320 tmp.img xlisp/*.lsp 0:
	cpmcp -f cpm86-320 tmp.img cpu/samples/t*.* 0:
	cpmls -F -f cpm86-320 tmp.img 
	python3 tools/cpm86twist.py twist tmp.img $@
	rm -f tmp.img



.PHONY: all clean $(SUBDIRS)

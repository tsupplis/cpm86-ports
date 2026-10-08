SUBDIRS=ansi2kr tinybas lispc xlisp yacc vtl2 filer dc grep cpm80 lbr \
    edlin sokoban fv seemem

all:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

clean:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done
	rm -f tmp.img cpmtest.img

test: cpmtest.img
	./cpm86 

cpmtest.img:  all
	cp cpmbase.img $@
	python3 tools/cpm86twist.py untwist $@ tmp.img
	cpmls -F -f cpm86-320 tmp.img 
	for i in */*.cmd;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	for i in */*.vtl;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	for i in */*.hlp;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	for i in */*.dat;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	for i in xlisp/*.lsp;do cpmcp -f cpm86-320 tmp.img $$i 0:;done
	cpmls -F -f cpm86-320 tmp.img 
	python3 tools/cpm86twist.py twist tmp.img $@
	rm -f tmp.img



.PHONY: all clean $(SUBDIRS)

SUBDIRS1=filer lbr fv seemem disk7 wash15 xsq uucp dc grep sed diff ndbm life kermit \
    crc banner tr uniq sort

SUBDIRS2=ansi2kr tinybas xlisp yacc vtl2 ratfor cpu diskstat sokoban edlin 
    
SUBDIRS3=trek setboot expr kilo

SUBDIRS=$(SUBDIRS1) $(SUBDIRS2) $(SUBDIRS3)
all:
	for d in $(SUBDIRS); do \
		$(MAKE) -j1 -C $$d $@ || exit $$?; \
	done

# Only projects with runnable automated suites or build checks belong here.
# Interactive PCE sessions are reserved for final-test.
TEST_SUBDIRS=$(filter edlin lbr xsq uucp dc grep sed diff ndbm tr uniq sort tinybas xlisp vtl2 ratfor sokoban,$(SUBDIRS))

test:
	python3 tools/run-tests.py --make "$(MAKE)" $(TEST_SUBDIRS)

# --------------------------------------------------------------------
# Binary zip
# --------------------------------------------------------------------

dist: ports.zip

ports.zip: clean all
	set -e; \
	mkdir -p tmp-dist; \
	trap 'rm -rf tmp-dist' EXIT INT TERM; \
	rm -f ports.zip; \
	for d in $(SUBDIRS); do \
	    for i in $$d/*.cmd; do cp $$i tmp-dist; done; \
	done; \
	cp vtl2/*.vtl tmp-dist; \
	cp tinybas/*.bas tmp-dist; \
	cp sokoban/*.hlp tmp-dist; \
	cp sokoban/*.dat tmp-dist; \
	cp xlisp/*.lsp tmp-dist; \
	cp ndbm/*.h tmp-dist; \
	cp ndbm/*.lib tmp-dist; \
	cp ratfor/*.r tmp-dist; \
	cp expr/tinyexpr.h tmp-dist; \
	cp expr/tinyexpr.lib tmp-dist; \
	(cd tmp-dist; zip -j ../ports.zip *)


clean:
	for d in $(SUBDIRS); do \
		$(MAKE) -j1 -C $$d $@ || exit $$?; \
	done
	rm -f tmp.img cpmtest1.img ports.zip

cpm86test: cpmtest1.img cpmtest2.img cpmtest3.img
	./cpm86 

ccpm86test: cpmtest1.img cpmtest2.img cpmtest3.img
	./ccpm86 

cpmtest1.img: all
	cp cpmbase.img $@
	python3 tools/cpm86twist.py untwist $@ tmp.img
	cpmls -F -f cpm86-320 tmp.img
	set -e; for d in $(SUBDIRS1); do \
	    for i in $$d/*.cmd; do cpmcp -f cpm86-320 tmp.img $$i 0:; done; \
	done
	cpmls -F -f cpm86-320 tmp.img
	python3 tools/cpm86twist.py twist tmp.img $@
	rm -f tmp.img


cpmtest2.img: all
	cp cpmbase.img $@
	python3 tools/cpm86twist.py untwist $@ tmp.img
	cpmls -F -f cpm86-320 tmp.img
	set -e; for d in $(SUBDIRS2); do \
	    for i in $$d/*.cmd; do cpmcp -f cpm86-320 tmp.img $$i 0:; done; \
	done
	set -e; for i in vtl2/*.vtl tinybas/*.bas; do cpmcp -f cpm86-320 tmp.img $$i 0:; done
	cpmcp -f cpm86-320 tmp.img xlisp/*.lsp 0:
	cpmcp -f cpm86-320 tmp.img sokoban/*.hlp 0:
	cpmcp -f cpm86-320 tmp.img sokoban/*.dat 0:
	cpmcp -f cpm86-320 tmp.img cpu/samples/ver.com 0:
	cpmls -F -f cpm86-320 tmp.img
	python3 tools/cpm86twist.py twist tmp.img $@
	rm -f tmp.img


cpmtest3.img: all
	cp cpmbase.img $@
	python3 tools/cpm86twist.py untwist $@ tmp.img
	cpmls -F -f cpm86-320 tmp.img
	set -e; for d in $(SUBDIRS3); do \
	    for i in $$d/*.cmd; do cpmcp -f cpm86-320 tmp.img $$i 0:; done; \
	done
	cpmls -F -f cpm86-320 tmp.img
	python3 tools/cpm86twist.py twist tmp.img $@
	rm -f tmp.img

.DELETE_ON_ERROR:
.PHONY: all clean test cpm86test ccpm86test dist $(SUBDIRS)

all:	ratfor

getopt.o: getopt.c
	zcc +cpm -DHAVE_CONFIG_H -I. -O2 -c -o getopt.o getopt.c

lookup.o: lookup.c
	zcc +cpm -DHAVE_CONFIG_H -I. -O2 -c -o lookup.o lookup.c

ratfor.o: ratfor.c
	zcc +cpm -DHAVE_CONFIG_H -I. -O2 -c -o ratfor.o ratfor.c

ratfor: ratfor.o lookup.o getopt.o
	zcc +cpm -O2 -o ratfor getopt.o lookup.o ratfor.o

clean:
	rm *.o

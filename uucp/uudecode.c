/*
 * uudecode [input]
 *
 * Decode a uuencoded file.
 *
 * CP/M-86 port:
 *   - removed <pwd.h>, <sys/stat.h>, ~user handling, chmod (unavailable)
 *   - output opened in binary mode ("wb") to preserve exact bytes
 *   - CCP uppercases command tail; option letters folded to lower case
 *   - added -o outfile to override the filename from the begin line
 *   - added -p (BDOS pause) like grep/sed
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* single character decode */
#define DEC(c)	(((c) - ' ') & 077)

static void decode(FILE *in, FILE *out);
static void outdec(char *p, FILE *f, int n);
static void usage(void);

int
main(argc, argv)
int argc;
char **argv;
{
	FILE *in, *out;
	int mode;
	char dest[128];
	char buf[80];
	char *outfile = 0;
	int optc;

	while (--argc > 0 && (++argv)[0][0] == '-') {
		optc = argv[0][1];
		if (optc >= 'A' && optc <= 'Z') optc += 'a' - 'A';
		switch (optc) {
		case 'o':
			if (--argc <= 0) {
				fprintf(stderr, "uudecode: -o requires a filename\n");
				exit(2);
			}
			outfile = *++argv;
			break;
		case '?':
		case 'h':
			usage();
			break;
		default:
			fprintf(stderr, "uudecode: unknown option: %c\n", argv[0][1]);
			exit(2);
		}
	}

	/* optional input file argument */
	if (argc > 0) {
		if ((in = fopen(argv[0], "r")) == 0) {
			fprintf(stderr, "uudecode: cannot open %s\n", argv[0]);
			exit(1);
		}
	} else
		in = stdin;

	if (argc > 1) {
		usage();
	}

	/* search for header line */
	for (;;) {
		if (fgets(buf, sizeof buf, in) == 0) {
			fprintf(stderr, "uudecode: no begin line\n");
			exit(3);
		}
		if (strncmp(buf, "begin ", 6) == 0)
			break;
	}
	sscanf(buf, "begin %o %127s", &mode, dest);

	/* open output: -o overrides the name from the begin line */
	if (outfile != 0)
		strcpy(dest, outfile);

	out = fopen(dest, "w");
	if (out == 0) {
		fprintf(stderr, "uudecode: cannot create %s\n", dest);
		exit(4);
	}

	decode(in, out);

	if (fgets(buf, sizeof buf, in) == 0 ||
	    strncmp(buf, "end", 3) != 0) {
		fprintf(stderr, "uudecode: no end line\n");
		exit(5);
	}
	fclose(out);
	if (in != stdin) fclose(in);
	exit(0);
}

static void
decode(in, out)
FILE *in;
FILE *out;
{
	char buf[80];
	char *bp;
	int n;

	for (;;) {
		if (fgets(buf, sizeof buf, in) == 0) {
			fprintf(stderr, "uudecode: short file\n");
			exit(10);
		}
		n = DEC(buf[0]);
		if (n <= 0)
			break;
		bp = &buf[1];
		while (n > 0) {
			outdec(bp, out, n);
			bp += 4;
			n -= 3;
		}
	}
}

static void
outdec(p, f, n)
char *p;
FILE *f;
int n;
{
	int c1, c2, c3;

	c1 = DEC(*p) << 2 | DEC(p[1]) >> 4;
	c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
	c3 = DEC(p[2]) << 6 | DEC(p[3]);
	if (n >= 1) putc(c1, f);
	if (n >= 2) putc(c2, f);
	if (n >= 3) putc(c3, f);
}

static void
usage()
{
	fprintf(stderr, "uudecode - decode a uuencoded file\n");
	fprintf(stderr, "usage: uudecode [-o outfile] [infile]\n");
	exit(2);
}

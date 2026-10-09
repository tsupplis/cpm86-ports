/*
 * uuencode [infile] remotefile
 *
 * Encode a file so it can be mailed to a remote system.
 *
 * CP/M-86 port:
 *   - removed fstat/stat (unavailable); mode hardcoded to 0644
 *   - input opened in binary mode ("rb") so ^Z does not truncate
 *   - CCP uppercases command tail; option letters folded to lower case
 *   - added -o outfile (freopen stdout) for use without pipes
 *   - added -p (BDOS pause) like grep/sed
 */
#include <stdio.h>
#include <stdlib.h>

/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) ((c) ? ((c) & 077) + ' ': '`')

static void encode(FILE *in, FILE *out);
static void outdec(char *p, FILE *f);
static int  fr(FILE *fd, char *buf, int cnt);
static void usage(void);

static int pflag = 0;
static int plines = 0;

static void
conputs(s)
char *s;
{
	while (*s) {
		if (pflag && *s == '\n') {
			if (++plines >= 23) {
				plines = 0;
				while (bdos(6, 0xFF) == 0)
					;
			}
		}
		bdos(2, *s++);
	}
}

int
main(argc, argv)
int argc;
char **argv;
{
	FILE *in;
	int optc;

	while (--argc > 0 && (++argv)[0][0] == '-') {
		optc = argv[0][1];
		if (optc >= 'A' && optc <= 'Z') optc += 'a' - 'A';
		switch (optc) {
		case 'o':
			if (--argc <= 0) {
				fprintf(stderr, "uuencode: -o requires a filename\n");
				exit(2);
			}
			if (freopen(*++argv, "w", stdout) == NULL) {
				fprintf(stderr, "uuencode: cannot open %s\n", *argv);
				exit(2);
			}
			break;
		case 'p':
			pflag++;
			break;
		case '?':
		case 'h':
			usage();
			break;
		default:
			fprintf(stderr, "uuencode: unknown option: %c\n", argv[0][1]);
			exit(2);
		}
	}

	/* optional input file argument */
	if (argc > 1) {
		if ((in = fopen(argv[0], "rb")) == NULL) {
			fprintf(stderr, "uuencode: cannot open %s\n", argv[0]);
			exit(1);
		}
		argv++; argc--;
	} else
		in = stdin;

	if (argc != 1) {
		usage();
	}

	/* mode hardcoded: no stat() on CP/M-86 */
	printf("begin 644 %s\n", argv[0]);

	encode(in, stdout);

	printf("end\n");
	if (in != stdin) fclose(in);
	exit(0);
}

static void
encode(in, out)
FILE *in;
FILE *out;
{
	char buf[45];
	int i, n;

	for (;;) {
		n = fr(in, buf, 45);
		putc(ENC(n), out);
		for (i = 0; i < n; i += 3)
			outdec(&buf[i], out);
		putc('\n', out);
		if (n <= 0)
			break;
	}
}

static void
outdec(p, f)
char *p;
FILE *f;
{
	int c1, c2, c3, c4;

	c1 = *p >> 2;
	c2 = (*p << 4) & 060 | (p[1] >> 4) & 017;
	c3 = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
	c4 = p[2] & 077;
	putc(ENC(c1), f);
	putc(ENC(c2), f);
	putc(ENC(c3), f);
	putc(ENC(c4), f);
}

static int
fr(fd, buf, cnt)
FILE *fd;
char *buf;
int cnt;
{
	int c, i;

	for (i = 0; i < cnt; i++) {
		c = getc(fd);
		if (c == EOF)
			return i;
		buf[i] = c;
	}
	return cnt;
}

static void
usage()
{
	fprintf(stderr, "uuencode - encode a binary file for mail transfer\n");
	fprintf(stderr, "usage: uuencode [-o outfile] [-p] [infile] remotefile\n");
	exit(2);
}

/*
 * diff - driver and subroutines (CP/M-86 port)
 */
#include "diff.h"

/* --- global variable definitions (declared extern in diff.h) --- */
int	opt;
int	tflag;
int	bflag;
int	wflag;
int	iflag;
int	wantelses;
char	*ifdef1;
char	*ifdef2;
char	*endifname;
int	inifdef;
int	context;
int	status;
int	anychange;
int	pflag;
int	plines;
int	oflag;
char	*file1, *file2;
struct	stat stb1, stb2;

#define PAGELEN 23

/* --- pageline/conputs (identical pattern to grep and sed) --- */
void
conputs(s)
char *s;
{
	while (*s)
		bdos(2, *s++);
}

void
pageline()
{
	int c;

	if (!pflag || ++plines < PAGELEN)
		return;
	plines = 0;
	conputs("-- Press a key to continue, ^C to quit --");
	while ((c = bdos(6, 0xFF) & 0xFF) == 0)
		continue;
	conputs("\r                                        \r");
	if (c == 3)
		exit(0);
}

/* --- casefold: same % toggle as grep/sed --- */
static char cftmp[512];

static void
casefold(astr)
char *astr;
{
	char *p, *s;
	int c, up;

	up = 0;
	for (s = cftmp, p = astr; *p; ) {
		if (s >= &cftmp[sizeof(cftmp)-2]) break;
		c = *p++;
		if (c == '\\') {
			if (*p == '%') *s++ = *p++;
			else { *s++ = c; if (*p) *s++ = *p++; }
			up = 0; continue;
		}
		if (c == '%') { up = !up; continue; }
		if (c >= 'A' && c <= 'Z')
			*s++ = up ? c : (char)(c + ('a' - 'A'));
		else if (c >= 'a' && c <= 'z')
			*s++ = up ? (char)(c - ('a' - 'A')) : c;
		else if ((c >= '0' && c <= '9') || c == '_')
			*s++ = c;
		else { *s++ = c; up = 0; }
	}
	*s = '\0';
	for (s = cftmp, p = astr; (*p++ = *s++); );
}

/* --- diff_stat: stub — try fopen to verify file exists --- */
int
diff_stat(path, sb)
char *path;
struct stat *sb;
{
	FILE *f;
	sb->st_mode = S_IFREG;
	f = fopen(path, "r");
	if (f == NULL)
		return(-1);
	fclose(f);
	return(0);
}

static void
usage()
{
	fprintf(stderr, "diff - compare two files\n");
	fprintf(stderr, "usage: diff [-bwite] [-c[N]] [-fn] [-o outfile] [-p] [-?] file1 file2\n\n");
	fprintf(stderr, "-b      ignore trailing blanks and compress other blanks\n");
	fprintf(stderr, "-w      ignore all blanks\n");
	fprintf(stderr, "-i      ignore case\n");
	fprintf(stderr, "-t      expand tabs on output\n");
	fprintf(stderr, "-e      produce an ed script\n");
	fprintf(stderr, "-f      produce a reverse ed script\n");
	fprintf(stderr, "-n      produce a numbered reverse ed script\n");
	fprintf(stderr, "-c[N]   produce context diff (default 3 lines)\n");
	fprintf(stderr, "-D name produce merged output with #ifdef NAME\n");
	fprintf(stderr, "-o file write output to file (CP/M-86: no pipes)\n");
	fprintf(stderr, "-p      pause after 23 lines (CP/M-86: no pipes; incompatible with -o)\n");
	fprintf(stderr, "-?      print this usage summary\n");
	exit(0);
}

void	noroom();

int
main(argc, argv)
	int argc;
	char **argv;
{
	register char *argp;
	int optc;

	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		argp = &argv[0][1];
		argv++, argc--;
		while (*argp) {
			/* fold option letter for CP/M-86 upper-case tail */
			optc = *argp++;
			if (optc >= 'A' && optc <= 'Z') optc += 'a' - 'A';
			switch (optc) {

			case 'd':
				/* -Dname: merged #ifdef output */
				wantelses = 1;
				ifdef1 = "";
				opt = D_IFDEF;
				ifdef2 = argp;
				*--argp = 0;
				continue;
			case 'e':
				opt = D_EDIT;
				continue;
			case 'f':
				opt = D_REVERSE;
				continue;
			case 'n':
				opt = D_NREVERSE;
				continue;
			case 'b':
				bflag = 1;
				continue;
			case 'w':
				wflag = 1;
				continue;
			case 'i':
				iflag = 1;
				continue;
			case 't':
				tflag = 1;
				continue;
			case 'c':
				opt = D_CONTEXT;
				if (isdigit(*argp)) {
					context = atoi(argp);
					while (isdigit(*argp))
						argp++;
					if (*argp) {
						fprintf(stderr,
						    "diff: -c: bad count\n");
						done(0);
					}
					argp = "";
				} else
					context = 3;
				continue;
			case 'o':
				/* -o outfile: redirect stdout to file */
				/* outfile is the *next* argv token */
				if (argc-- <= 0) {
					fprintf(stderr, "diff: -o requires a filename\n");
					done(0);
				}
				if (freopen(*argv, "w", stdout) == NULL) {
					fprintf(stderr, "diff: cannot open output file: %s\n", *argv);
					done(0);
				}
				argv++;
				pflag = 0;	/* -o and -p incompatible; -o wins */
				oflag++;
				continue;
			case 'p':
				pflag++;
				continue;
			case '?':
				usage();
				continue;
			default:
				fprintf(stderr, "diff: -%c: unknown option\n", optc);
				done(0);
			}
		}
	}
	if (argc != 2) {
		fprintf(stderr, "diff: two filename arguments required\n");
		done(0);
	}
	file1 = argv[0];
	file2 = argv[1];
	if (stat(file1, &stb1) < 0) {
		fprintf(stderr, "diff: cannot access %s\n", file1);
		done(0);
	}
	if (stat(file2, &stb2) < 0) {
		fprintf(stderr, "diff: cannot access %s\n", file2);
		done(0);
	}
	/* directory diff not supported on CP/M-86 */
	if ((stb1.st_mode & S_IFMT) == S_IFDIR ||
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		fprintf(stderr, "diff: directory comparison not supported\n");
		done(0);
	}
	diffreg();
	done(0);
}

char *
savestr(cp)
	register char *cp;
{
	register char *dp = malloc(strlen(cp)+1);

	if (dp == 0) {
		fprintf(stderr, "diff: ran out of memory\n");
		done(0);
	}
	strcpy(dp, cp);
	return (dp);
}

int
min(a,b)
	int a,b;
{
	return (a < b ? a : b);
}

int
max(a,b)
	int a,b;
{
	return (a > b ? a : b);
}

void
done(sig)
	int sig;
{
	exit(status);
}

char *
talloc(n)
	int n;
{
	register char *p;

	if ((p = malloc((unsigned)n)) != NULL)
		return(p);
	noroom();
}

char *
ralloc(p,n)
	char *p;
	int n;
{
	register char *q;

	if ((q = realloc(p, (unsigned)n)) == NULL)
		noroom();
	return(q);
}

void
noroom()
{
	fprintf(stderr, "diff: files too big, try smaller files\n");
	done(0);
}

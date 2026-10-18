/*
 * Deal with duplicated lines in a file
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


#define PAGELEN 23
int pflag, plines;
/* Same direct-console paging convention as sed/sed1.c. */
static void
conputs(s)
char *s;
{
    while (*s) bdos(2, *s++);
}

static void
pageline()
{
    int c;
    if (!pflag || ++plines < PAGELEN)
        return;
    plines = 0;
    fflush(stdout);
    conputs("[more]");
    while ((c = bdos(6, 0xFF) & 0xFF) == 0)
        continue;
    conputs("\r                                        \r");
    if (c == 3) exit(0);
}

void usage();
int fields;
int letters;
int linec;
char    mode;
int uniq;

int      gline(char buf[]);
void     pline(char buf[]);
int      equal(char b1[], char b2[]);
char    *skip(char *s);
void     printe(char *p, char *s);

int
main(argc, argv)
int argc;
char *argv[];
{
    static char b1[1000], b2[1000];

    char *out, *input, *opt;
    int c;
    out = input = 0;
    while (--argc > 0) {
        opt = *++argv;
        if (*opt == '-' && opt[1]) {
            if (isdigit(opt[1])) { fields = atoi(opt+1); continue; }
            while (*++opt) {
                c = *opt;
                if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
                switch (c) {
                case 'c': case 'd': case 'u': mode = c; break;
                case 'p': pflag = 1; break;
                case 'o':
                    if (--argc <= 0) usage(1);
                    out = *++argv; break;
                case '?': usage(0); break;
                default: usage(1);
                }
            }
        } else if (*opt == '+') letters = atoi(opt+1);
        else if (!input) input = opt;
        else if (!out) out = opt;
        else usage(1);
    }
    if (input && freopen(input, "r", stdin) == NULL)
        printe("cannot open %s\n", input);
    if (out) {
        pflag = 0;
        if (freopen(out, "w", stdout) == NULL)
            printe("cannot create %s\n", out);
    }

    if(gline(b1))
        exit(0);
    for(;;) {
        linec++;
        if(gline(b2)) {
            pline(b1);
            exit(0);
        }
        if(!equal(b1, b2)) {
            pline(b1);
            linec = 0;
            do {
                linec++;
                if(gline(b1)) {
                    pline(b2);
                    exit(0);
                }
            } while(equal(b1, b2));
            pline(b2);
            linec = 0;
        }
    }
}

int
gline(buf)
register char buf[];
{
    register int c;
    int n;
    static int ended;
    /* Aztec agetc may resume after a text EOF marker on another call. */
    if (ended) return(1);
    n = 0;
    while ((c = getchar()) != '\n' && c != EOF) {
        if (n >= 999) printe("uniq: line too long%s\n", "");
        buf[n++] = c;
    }
    buf[n] = 0;
    if (c == EOF) ended = 1;
    return(c == EOF && n == 0);
}

void
pline(buf)
register char buf[];
{
    switch(mode) {

    case 'u':
        if(uniq) {
            uniq = 0;
            return;
        }
        break;

    case 'd':
        if(uniq) break;
        return;

    case 'c':
        printf("%4d ", linec);
    }
    uniq = 0;
    fputs(buf, stdout);
    putchar('\n');
    if (ferror(stdout)) exit(1);
    pageline();
}

int
equal(b1, b2)
register char b1[], b2[];
{
    register char c;

    b1 = skip(b1);
    b2 = skip(b2);
    while((c = *b1++) != 0)
        if(c != *b2++) return(0);
    if(*b2 != 0)
        return(0);
    uniq++;
    return(1);
}

char *
skip(s)
register char *s;
{
    register int nf, nl;

    nf = nl = 0;
    while(nf++ < fields) {
        while(*s == ' ' || *s == '\t')
            s++;
        while( !(*s == ' ' || *s == '\t' || *s == 0) )
            s++;
    }
    while(nl++ < letters && *s != 0)
            s++;
    return(s);
}

void
printe(p,s)
char *p,*s;
{
    fprintf(stderr, p, s);
    exit(1);
}

void
usage(rc)
int rc;
{
    fprintf(stderr, "usage: uniq [-cdu] [-p] [-o file] [-fields] [+chars] [input [output]]\n");
    fprintf(stderr, "-c count, -d repeated only, -u unique only, -p page, -? help\n");
    exit(rc);
}

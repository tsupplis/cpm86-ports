/*
 * tr - transliterate data stream
 */
#include <stdio.h>
#include <stdlib.h>


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

int dflag   = 0;
int sflag   = 0;
int cflag = 0;
int save    = 0;
char    code[256];
char    squeez[256];
char    vect[256];
struct string { int last, max; char *p; } string1, string2;

void usage();
void casefold();
int     next(struct string *);
int     nextc(struct string *);

int
main(argc,argv)
int argc;
char **argv;
{
    register int i;
    int j;
    register int c, d;
    char *compl;
    int lastd;

    string1.last = string2.last = 0;
    string1.max = string2.max = 0;
    string1.p = string2.p = "";

    {
        char *out, *opt;
        out = 0;
        while (argc > 1 && argv[1][0] == '-' && argv[1][1]) {
            opt = *++argv; --argc;
            if (opt[1] == '-' && opt[2] == 0) break;
            while (*++opt) {
                i = *opt;
                if (i >= 'A' && i <= 'Z') i += 'a' - 'A';
                switch (i) {
                case 'c': cflag = 1; break;
                case 'd': dflag = 1; break;
                case 's': sflag = 1; break;
                case 'p': pflag = 1; break;
                case 'o':
                    if (argc <= 1) usage(1);
                    out = *++argv; --argc;
                    break;
                case '?': usage(0); break;
                default: usage(1);
                }
            }
        }
        --argc; ++argv;
        if (argc < 1 || argc > 3) usage(1);
        casefold(argv[0]);
        string1.p = argv[0];
        if (argc > 1) {
            casefold(argv[1]);
            string2.p = argv[1];
        }
        if (argc > 2 && freopen(argv[2], "r", stdin) == NULL) {
            fprintf(stderr, "tr: cannot open %s\n", argv[2]); exit(1);
        }
        if (out) {
            pflag = 0;
            if (freopen(out, "w", stdout) == NULL) {
                fprintf(stderr, "tr: cannot create %s\n", out); exit(1);
            }
        }
    }
    for(i=0; i<256; i++)
        code[i] = vect[i] = 0;
    if(cflag) {
        while(c = next(&string1))
            vect[c&0377] = 1;
        j = 0;
        for(i=1; i<256; i++)
            if(vect[i]==0) vect[j++] = i;
        vect[j] = 0;
        compl = vect;
    }
    for(i=0; i<256; i++)
        squeez[i] = 0;
    lastd = 0;
    for(;;){
        if(cflag) c = *compl++;
        else c = next(&string1);
        if(c==0) break;
        d = next(&string2);
        if(d==0) d = lastd;
        else lastd = d;
        squeez[d&0377] = 1;
        code[c&0377] = dflag?1:d;
    }
    while(d = next(&string2))
        squeez[d&0377] = 1;
    squeez[0] = 1;
    for(i=0;i<256;i++) {
        if(code[i]==0) code[i] = i;
        else if(dflag) code[i] = 0;
    }

    clearerr(stdout);
    while((c=getchar()) != EOF ) {
        if(c == 0) continue;
        if(c = code[c&0377]&0377)
            if(!sflag || c!=save || !squeez[c&0377]) {
                putchar(save = c);
                if (c == '\n') pageline();
                if(ferror(stdout))
                    exit(1);
            }
    }
    exit(0);
}

int
next(s)
struct string *s;
{
again:
    if(s->max) {
        if(s->last++ < s->max)
            return(s->last);
        s->max = s->last = 0;
    }
    if(s->last && *s->p=='-') {
        nextc(s);
        s->max = nextc(s);
        if(s->max==0) {
            s->p--;
            return('-');
        }
        if(s->max < s->last)  {
            s->last = s->max-1;
            return('-');
        }
        goto again;
    }
    return(s->last = nextc(s));
}

int
nextc(s)
struct string *s;
{
    register int c, i, n;

    c = *s->p++;
    if(c=='\\') {
        i = n = 0;
        while(i<3 && (c = *s->p)>='0' && c<='7') {
            n = n*8 + c - '0';
            i++;
            s->p++;
        }
        if(i>0) c = n;
        else c = *s->p++;
    }
    if(c==0) --s->p;
    return(c&0377);
}

void
usage(rc)
int rc;
{
    fprintf(stderr, "usage: tr [-cdsp] [-o file] string1 [string2 [input]]\n");
    fprintf(stderr, "-c complement, -d delete, -s squeeze, -p page, -? help\n");
    fprintf(stderr, "Sets: letters default to lower case; %% toggles case within a word\n");
    fprintf(stderr, "      A-Z means a-z; %%A-%%Z means A-Z; \\%% means literal %%\n");
    exit(rc);
}

/* sed's command-tail case layer, applied only to the two character sets.
 * Folding never increases length, so it can safely be done in place. */
void
casefold(astr)
char *astr;
{
    char *p, *s;
    int c, up;
    up = 0;
    for (p = s = astr; *p; ) {
        c = *p++;
        if (c == '\\') {
            if (*p == '%') *s++ = *p++;
            else {
                *s++ = c;
                if (*p) *s++ = *p++;
            }
            up = 0;
            continue;
        }
        if (c == '%') { up = !up; continue; }
        if (c >= 'A' && c <= 'Z')
            *s++ = up ? c : c + ('a'-'A');
        else if (c >= 'a' && c <= 'z')
            *s++ = up ? c - ('a'-'A') : c;
        else {
            *s++ = c;
            if (!((c >= '0' && c <= '9') || c == '_')) up = 0;
        }
    }
    *s = 0;
}

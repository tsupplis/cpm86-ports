/*< 
  ratfor77
 
  Usage
  ratfor77 < input.r >output.f pars
 
 
	Input Parameters
  l - flag [-l n]  user sets starting label n
  o - flag [-o output]  specify output file, otherwise it is stdout
  C - flag [-C] keep comments in (useful for compiler directives)

	
  
 Description 
  A ratfor pre-processor in C.

Comments
  Derived from a pre-processor distributed by the
  University of Arizona. Closely corresponds to the
  pre-processor described in the "SOFTWARE TOOLS" book.

Category
 Tools

Compile Level
 DISTR
 
>*/
/*
 *By: oz
 *
 * Not derived from AT&T code.
 *
 * This code is in the public domain. In other words, all rights
 * are granted to all recipients, "public" at large.
 *
 * Modification history:
 *
 * June 1985
 *	- Ken Yap's mods for F77 output. Currently
 *	  available thru #define F77.
 *	- Two minor bug-fixes for sane output.
 * June 1985
 *	- Improve front-end with getopt().
 *	  User may specify -l n for starting label.
 *	- Retrofit switch statement handling. This code
 *	  is borrowed from the SWTOOLS Ratfor.
 *
 * 05-28-91 W. Bauske IBM
 *	- ported to RS/6000
 *	- fixed line continuations
 *	- added -C option to leave comments in the source code
 *	- added % in column 1 to force copy to output
 *	- support both && and & for .and.
 *	- support both || and | for .or.
 *
 * 11-15-95 Emilio Lopes <ecl@if.usp.br> (Search for ECL to see my changes)
 *      - Option -l now works (was disabled by some reason).
 *      - Removed from usage message the options that are not yet
 *        implemented.
 *      - Added option -?, that shows usage message.
 *
 * 06-08-05 Alan Bain <alanb@chiark.greenend.org.uk> (AFRB tags changes)
 *      - Fixed handling of include file to cope with 
 *        absence of quotes (as per original ratfor specification)
 *        so now support include file, include 'file' and include "file"
 *        syntax.
 *        
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ratdef.h"
#include "ratcom.h"
#include "lookup.h"
#include "config.h"
#include "getopt.h"

#ifndef VERSION
#define VERSION "(undefined)"
#endif

/* keywords: */

char sdo[3] = {
	LETD,LETO,EOS};
char vdo[2] = {
	LEXDO,EOS};

char sif[3] = {
	LETI,LETF,EOS};
char vif[2] = {
	LEXIF,EOS};

char selse[5] = {
	LETE,LETL,LETS,LETE,EOS};
char velse[2] = {
	LEXELSE,EOS};

#ifdef F77
char sthen[5] = {
	LETT,LETH,LETE,LETN,EOS};

char sendif[6] = {
	LETE,LETN,LETD,LETI,LETF,EOS};

#endif /* F77 */
char swhile[6] = {
	LETW, LETH, LETI, LETL, LETE, EOS};
char vwhile[2] = {
	LEXWHILE, EOS};

char ssbreak[6] = {
	LETB, LETR, LETE, LETA, LETK, EOS};
char vbreak[2] = {
	LEXBREAK, EOS};

char snext[5] = {
	LETN,LETE, LETX, LETT, EOS};
char vnext[2] = {
	LEXNEXT, EOS};

char sfor[4] = {
	LETF,LETO, LETR, EOS};
char vfor[2] = {
	LEXFOR, EOS};

char srept[7] = {
	LETR, LETE, LETP, LETE, LETA, LETT, EOS};
char vrept[2] = {
	LEXREPEAT, EOS};

char suntil[6] = {
	LETU, LETN, LETT, LETI, LETL, EOS};
char vuntil[2] = {
	LEXUNTIL, EOS};

char sswitch[7] = {
	LETS, LETW, LETI, LETT, LETC, LETH, EOS};
char vswitch[2] = {
	LEXSWITCH, EOS};

char scase[5] = {
	LETC, LETA, LETS, LETE, EOS};
char vcase[2] = {
	LEXCASE, EOS};

char sdefault[8] = {
	LETD, LETE, LETF, LETA, LETU, LETL, LETT, EOS};
char vdefault[2] = {
	LEXDEFAULT, EOS};

char sret[7] = {
	LETR, LETE, LETT, LETU, LETR, LETN, EOS};
char vret[2] = {
	LEXRETURN, EOS};

char sstr[7] = {
	LETS, LETT, LETR, LETI, LETN, LETG, EOS};
char vstr[2] = {
	LEXSTRING, EOS};

char deftyp[2] = {
	DEFTYPE, EOS};

/* constant strings */

const char *errmsg = "error at line ";
const char *in     = " in ";
const char *ifnot  = "if(.not.";
const char *incl   = "include";
const char *fncn   = "function";
const char *def    = "define";
const char *bdef   = "DEFINE";
const char *contin = "continue";
const char *rgoto  = "goto ";
const char *dat    = "data ";
const char *eoss   = "EOS/";

char stelem[128] = "integer";	/* element type of arrays generated for string directive */

extern char ngetch(char *c, FILE *fd);
char *progname;
int startlab = 23000;		/* default start label */
int leaveC = NO;		/* Flag for handling comments */

/* prototypes */
int alldig(const char *str);
void baderr(const char *msg);
void balpar(void);
void brknxt(int sp, int *lextyp, int *labval, int token);
void cascod (int lab, int token);
int caslab (int *n, int *t);
int ctoi(char *in_, int *i);
int deftok(char *token, int toksiz, FILE *fd);
void docode(int *lab);
void dostat(int lab);
void eatup(void);
void elseif(int lab);
int equal(const char *str1, const char *str2);
void error(const char *msg, const char *s);
void fold(char *token);
void forcod(int *lab);
void fors(int lab);
void getdef(char *token, int toksiz, char *defn, int defsiz, FILE *fd);
int gettok(char *token, int toksiz);
int gnbtok(char *token, int toksiz);
int gtok(char *lexstr, int toksiz, FILE *fd);
void ifcode(int *lab);
void ifend(void);
void ifgo(int lab);
void ifthen(void);
void initvars(void);
int itoc(int n, char *str, int size);
void labelc(const char *lexstr);
int labgen(int n);
int lex(char *lexstr);
int look(const char *name,char *defn);
void otherc(const char *lexstr);
void outasis(FILE *fd);
void outch(char c);
void outcmnt(FILE *fd);
void outcon(int n);
void outdon(void);
void outgo(int n);
void outnum(int n);
void outstr(const char *str);
void outtab(void);
void parse(void);
void pbstr(const char *string);
void putbak(char c);
int relate(char *token, FILE *fd);
void repcod(int *lab);
void retcod(void);
void scopy(const char *from, int i, char *to, int j);
void skpblk(FILE *fd);
void strdcl(void);
void swcode (int *lab);
void swend(int lab);
void swvar (int lab);
void synerr(const char *msg);
int type(signed char c);
void unstak(int *sp, const int *lextyp, const int *labval, char token);
void untils(int lab, int token);
void whilec(int *lab);
void whiles(int lab);

/*
 * M A I N   L I N E  &  I N I T
 */

long heap;

int main(int argc,char **argv)
{
	int c, errflg = 0;

	progname = argv[0];

/* ECL	while ((c=our_getopt(argc, argv, "Chn:l:o:6:")) != EOF) */
	while ((c=our_getopt(argc, argv, "C?n:l:O:s:")) != EOF)
	switch (c) {
		case 'C':
			leaveC = YES; /* keep comments in src */
			break;
		case 'h':
				/* not written yet */
			break;
		case 'l':	/* user sets label */
			if (optarg != NULL)
				startlab = atoi(optarg);
			else {
				fprintf(stderr,
					"-l requires an integer argument\n");
				exit(1);
			}
			break;
		case 'O':
			if ((freopen(optarg, "w", stdout)) == NULL)
				error("can't write %s\n", optarg);
			break;
		case 's':
			if (optarg != NULL) {
			        if (strlen(optarg) > sizeof(stelem) - 1) {
				        fprintf(stderr, "-s argument too long\n");
					exit(1);
				}
			        strncpy(stelem, optarg, sizeof(stelem) - 1);
				stelem[sizeof(stelem) - 1] = '\0';
			} else {
				fprintf(stderr, "-s requires a type name argument\n");
				exit(1);
			}
		case '6':
				/* not written yet */
			break;
                case '?':                    /* ECL */
                        ++errflg;
                        break;
		default:
			++errflg;
	}

	if (errflg) {
		fprintf(stderr,
/* ECL		"usage: %s [-C][-hx][-l n][-o file][-6x] [file...]\n",progname); */
		"usage: %s [-?] [-C] [-l n] [-o file] [file...]\n",progname);
		exit(1);
	}

	/*
	 * present version can only process one file, sadly.
	 */
	if (optind77 >= argc)
		infile[0] = stdin;
	else if ((infile[0] = fopen(argv[optind77], "r")) == NULL)
		error("cannot read %s\n", argv[optind77]);

	initvars();

	parse();		/* call parser.. */

	exit(0);
}

/*
 * initialise
 */
void initvars(void)
{
	int i;

	outp = 0;		/* output character pointer */
	level = 0;		/* file control */
	linect[0] = 1;		/* line count of first file */
	fnamp = 0;
	fnames[0] = EOS;
	bp = -1;		/* pushback buffer pointer */
	fordep = 0;		/* for stack */
	swtop = 0;		/* switch stack index */
	swlast = 1;		/* switch stack index */
	for( i = 0; i <= 126; i++)
		tabptr[i] = 0;
	install(def, deftyp);	/* default definitions */
	install(bdef, deftyp);
	fcname[0] = EOS;	/* current function name */
	label = startlab;	/* next generated label */
	printf("C Output from Public domain Ratfor, version %s\n", VERSION);
}

/*
 * P A R S E R
 */

void parse(void)
{
	char lexstr[MAXTOK];
	int lab, labval[MAXSTACK], lextyp[MAXSTACK], sp, i, token;

	sp = 0;
	lextyp[0] = EOF;
	for (token = lex(lexstr); token != EOF; token = lex(lexstr)) {
		if (token == LEXIF)
			ifcode(&lab);
		else if (token == LEXDO)
			docode(&lab);
		else if (token == LEXWHILE)
			whilec(&lab);
		else if (token == LEXFOR)
			forcod(&lab);
		else if (token == LEXREPEAT)
			repcod(&lab);
		else if (token == LEXSWITCH)
			swcode(&lab);
		else if (token == LEXCASE || token == LEXDEFAULT) {
			for (i = sp; i >= 0; i--)
				if (lextyp[i] == LEXSWITCH)
					break;
			if (i < 0)
				synerr("illegal case of default.");
			else
				cascod(labval[i], token);
		}
		else if (token == LEXDIGITS)
			labelc(lexstr);
		else if (token == LEXELSE) {
			if (lextyp[sp] == LEXIF)
				elseif(labval[sp]);
			else
				synerr("illegal else.");
		}
		if (token == LEXIF || token == LEXELSE || token == LEXWHILE
		    || token == LEXFOR || token == LEXREPEAT
		    || token == LEXDO || token == LEXDIGITS
		    || token == LEXSWITCH || token == LBRACE) {
			sp++;         /* beginning of statement */
			if (sp > MAXSTACK)
				baderr("stack overflow in parser.");
			lextyp[sp] = token;     /* stack type and value */
			labval[sp] = lab;
		}
		else if (token != LEXCASE && token != LEXDEFAULT) {
			/*
		         * end of statement - prepare to unstack
			 */
			if (token == RBRACE) {
				if (lextyp[sp] == LBRACE)
					sp--;
				else if (lextyp[sp] == LEXSWITCH) {
					swend(labval[sp]);
					sp--;
				}
				else
					synerr("illegal right brace.");
			}
			else if (token == LEXOTHER)
				otherc(lexstr);
			else if (token == LEXBREAK || token == LEXNEXT)
				brknxt(sp, lextyp, labval, token);
			else if (token == LEXRETURN)
				retcod();
		 	else if (token == LEXSTRING)
				strdcl();
			token = lex(lexstr);      /* peek at next token */
			pbstr(lexstr);
			unstak(&sp, lextyp, labval, token);
		}
	}
	if (sp != 0)
		synerr("unexpected EOF.");
}

/*
 * L E X I C A L  A N A L Y S E R
 */

/*
 *  alldig - return YES if str is all digits
 *
 */
int alldig(const char *str)
{
	int i,j;

	j = NO;
	if (str[0] == EOS)
		return(j);
	for (i = 0; str[i] != EOS; i++)
		if (type(str[i]) != DIGIT)
			return(j);
	j = YES;
	return(j);
}


/*
 * balpar - copy balanced paren string
 *
 */
void balpar(void)
{
	char token[MAXTOK];
	int t,nlpar;

	if (gnbtok(token, MAXTOK) != LPAREN) {
		synerr("missing left paren.");
		return;
	}
	outstr(token);
	nlpar = 1;
	do {
		t = gettok(token, MAXTOK);
		if (t==SEMICOL || t==LBRACE || t==RBRACE || t==EOF) {
			pbstr(token);
			break;
		}
		if (t == NEWLINE)      /* delete newlines */
			token[0] = EOS;
		else if (t == LPAREN)
			nlpar++;
		else if (t == RPAREN)
			nlpar--;
		/* else nothing special */
		outstr(token);
	}
	while (nlpar > 0);
	if (nlpar != 0)
		synerr("missing parenthesis in condition.");
}

/*
 * deftok - get token; process macro calls and invocations
 *
 */
int deftok(char *token, int toksiz, FILE *fd)
{
	char defn[MAXDEF];
	int t;
	for (t=gtok(token, toksiz, fd); t!=EOF; t=gtok(token, toksiz, fd)) {
		if (t != ALPHA)   /* non-alpha */
			break;
		if (look(token, defn) == NO)   /* undefined */
			break;
		if (defn[0] == DEFTYPE) {   /* get definition */
			getdef(token, toksiz, defn, MAXDEF, fd);
			install(token, defn);
		}
		else
			pbstr(defn);   /* push replacement onto input */
	}
	if (t == ALPHA)   /* convert to single case */
		fold(token);
	return(t);
}

/*
 * eatup - process rest of statement; interpret continuations
 *
 */
void eatup(void)
{
	char ptoken[MAXTOK], token[MAXTOK];
	int nlpar, t;

	nlpar = 0;
	do {
		t = gettok(token, MAXTOK);
		if (t == SEMICOL || t == NEWLINE)
			break;
		if (t == RBRACE || t == LBRACE) {
			pbstr(token);
			break;
		}
		if (t == EOF) {
			synerr("unexpected EOF.");
			pbstr(token);
			break;
		}
		if (t == COMMA || t == PLUS
			       || t == MINUS || t == STAR || t == LPAREN
		               || t == AND || t == BAR || t == BANG
			       || t == EQUALS || t == UNDERLINE ) {
			while (gettok(ptoken, MAXTOK) == NEWLINE)
				;
			pbstr(ptoken);
			if (t == UNDERLINE)
				token[0] = EOS;
		}
		if (t == LPAREN)
			nlpar++;
		else if (t == RPAREN)
			nlpar--;
		outstr(token);

	} while (nlpar >= 0);

	if (nlpar != 0)
		synerr("unbalanced parentheses.");
}

/*
 * getdef (for no arguments) - get name and definition
 *
 */
void getdef(char *token, int toksiz, char *defn, int defsiz, FILE *fd)
{
	int i = 0, nlpar, t;
	char c, ptoken[MAXTOK];

	skpblk(fd);
	/*
	 * define(name,defn) or
	 * define name defn
	 *
	 */
	if ((t = gtok(ptoken, MAXTOK, fd)) != LPAREN) {
		t = BLANK;              /* define name defn */
		pbstr(ptoken);
	}
	skpblk(fd);
	if (gtok(token, toksiz, fd) != ALPHA)
		baderr("non-alphanumeric name.");
	skpblk(fd);
	c = (char) gtok(ptoken, MAXTOK, fd);
	if (t == BLANK) {         /* define name defn */
		pbstr(ptoken);
		i = 0;
		do {
			c = ngetch(&c, fd);
			if (i > defsiz)
				baderr("definition too long.");
			defn[i++] = c;
		}
		while (c != SHARP && c != NEWLINE && c != (char)EOF && c != PERCENT);
		if (c == SHARP || c == PERCENT)
			putbak(c);
	}
	else if (t == LPAREN) {   /* define (name, defn) */
		if (c != COMMA)
			baderr("missing comma in define.");
		/* else got (name, */
		nlpar = 0;
		for (i = 0; nlpar >= 0; i++)
			if (i > defsiz)
				baderr("definition too long.");
			else if (ngetch(&defn[i], fd) == (char)EOF)
				baderr("missing right paren.");
			else if (defn[i] == LPAREN)
				nlpar++;
			else if (defn[i] == RPAREN)
				nlpar--;
		/* else normal character in defn[i] */
	}
	else
		baderr("getdef is confused.");
	if (i > 0)
		defn[i-1] = EOS;
}

/*
 * gettok - get token. handles file inclusion and line numbers
 *
 */
int gettok(char *token, int toksiz)
{
	int t, i;
	int tok;
	char name[MAXNAME];

	for ( ; level >= 0; level--) {
		for (tok = deftok(token, toksiz, infile[level]); tok != EOF;
		     tok = deftok(token, toksiz, infile[level])) {
			if (equal(token, fncn) == YES) {
				skpblk(infile[level]);
				t = deftok(fcname, MAXNAME, infile[level]);
				pbstr(fcname);
				if (t != ALPHA)
					synerr("missing function name.");
				putbak(BLANK);
				return(tok);
			}
			else if (equal(token, incl) == NO)
				return(tok);
			for (i = 0 ;; i = strlen((char *) (&name[0]))) {
				t = deftok(&name[i], MAXNAME, infile[level]);
				if (t == NEWLINE || t == SEMICOL) {
					pbstr(&name[i]);
					break;
				}
			}
			name[i] = EOS;
/*WSB 6-25-91
			if (name[1] == SQUOTE) {
				outtab();
				outstr(token);
				outstr(name);
				outdon();
				eatup();
				return(tok);
			}
*/
			if (level >= NFILES)
				synerr("includes nested too deeply.");
			else {
/**/
				/* AFRB 6-7-2005 */
				if ((name[1] == SQUOTE && name[i-1]==SQUOTE)
						|| (name[1]==DQUOTE && name[i-1]==DQUOTE)) {
					/* Truncation to remove quotes */
					name[i-1]=EOS;
					infile[level+1] = fopen((char*)&name[2], "r");
				} else {
					/* No quote syntax */
					infile[level+1] = fopen ((char*) &name[1], "r");
				}
				/*WSB 6-25-91
				  infile[level+1] = fopen(name, "r");
				  */
				linect[level+1] = 1;
				if (infile[level+1] == NULL)
					synerr("can't open include.");
				else {
					level++;
					if (fnamp + i <= MAXFNAMES) {
						scopy(name, 0, fnames, fnamp);
						fnamp = fnamp + i;    /* push file name stack */
					}
				}
			}
		}
		if (level > 0) {      /* close include and pop file name stack */
			fclose(infile[level]);
			for (fnamp--; fnamp > 0; fnamp--)
				if (fnames[fnamp-1] == EOS)
					break;
		}
	}
	token[0] = EOF;   /* in case called more than once */
	token[1] = EOS;
	tok = EOF;
	return(tok);
}

/*
 * gnbtok - get nonblank token
 *
 */
int gnbtok(char *token, int toksiz)
{
	int tok;

	if (level < 0) return EOF;
	skpblk(infile[level]);
	tok = gettok(token, toksiz);
	return(tok);
}

/*
 * gtok - get token for Ratfor
 *
 */
int gtok(char *lexstr, int toksiz, FILE *fd)
{
	int i, b, n, tok;
	char c;
	c = ngetch(&lexstr[0], fd);
	if (c == BLANK || c == TAB) {
		lexstr[0] = BLANK;
		while (c == BLANK || c == TAB)    /* compress many blanks to one */
			c = ngetch(&c, fd);
		if (c == PERCENT) {
			  outasis(fd);		/* copy direct to output if % */
			  c = NEWLINE;
		}
		if (c == SHARP) {
			if (leaveC == YES) {
			        outcmnt(fd);		/* copy comments to output */
			        c = NEWLINE;
			} else {
			        while (ngetch(&c, fd) != NEWLINE) /* strip comments */
				        ;
			}
		}
/*
		if (c == UNDERLINE)	
			if(ngetch(&c, fd) == NEWLINE)
				while(ngetch(&c, fd) == NEWLINE)
					;
			else
			{
				putbak(c);
				c = UNDERLINE;
			}
*/
		if (c != NEWLINE)
			putbak(c);
		else
			lexstr[0] = NEWLINE;
		lexstr[1] = EOS;
		return((int)lexstr[0]);
	}
	i = 0;
	tok = type(c);
	if (tok == LETTER) {	/* alpha */
		for (i = 0; i < toksiz - 3; i++) {
			tok = type(ngetch(&lexstr[i+1], fd));
			/* Test for DOLLAR added by BM, 7-15-80 */
			if (tok != LETTER && tok != DIGIT
			    && tok != UNDERLINE && tok!=DOLLAR
			    && tok != PERIOD)
				break;
		}
		putbak(lexstr[i+1]);
		tok = ALPHA;
	}
	else if (tok == DIGIT) {	/* digits */
		b = c - DIG0;	/* in case alternate base number */
		for (i = 0; i < toksiz - 3; i++) {
			if (type(ngetch(&lexstr[i+1], fd)) != DIGIT)
				break;
			b = 10*b + lexstr[i+1] - DIG0;
		}
		if (lexstr[i+1] == RADIX && b >= 2 && b <= 36) {
			/* n%ddd... */
			for (n = 0;; n = b*n + c - DIG0) {
				c = ngetch(&lexstr[0], fd);
				if (c >= LETA && c <= LETZ)
					c = c - LETA + DIG9 + 1;
				else if (c >= BIGA && c <= BIGZ)
					c = c - BIGA + DIG9 + 1;
				if (c < DIG0 || c >= DIG0 + b)
					break;
			}
			putbak(lexstr[0]);
			i = itoc(n, lexstr, toksiz);
		}
		else
			putbak(lexstr[i+1]);
		tok = DIGIT;
	}
#ifdef SQUAREB
	else if (c == LBRACK) {   /* allow [ for { */
		lexstr[0] = LBRACE;
		tok = LBRACE;
	}
	else if (c == RBRACK) {   /* allow ] for } */
		lexstr[0] = RBRACE;
		tok = RBRACE;
	}
#endif
	else if (c == SQUOTE || c == DQUOTE) {
		for (i = 1; ngetch(&lexstr[i], fd) != lexstr[0]; i++) {
			if (lexstr[i] == UNDERLINE) {
				if (ngetch(&c, fd) == NEWLINE) {
					while (c == NEWLINE || c == BLANK || c == TAB) {
						c = ngetch(&c, fd);
					}
					lexstr[i] = c;
				} else {
					putbak(c);
				}
			}
			if (lexstr[i] == NEWLINE || i >= toksiz-1) {
				synerr("missing quote.");
				lexstr[i] = lexstr[0];
				putbak(NEWLINE);
				break;
			}
		}
	}
	else if (c == PERCENT) {
		outasis(fd);		/* direct copy of protected */
		tok = NEWLINE;
	}
	else if (c == SHARP) { 
		if (leaveC == YES)
		        outcmnt(fd);		/* copy comments to output */
		else
		        while (ngetch(&lexstr[0], fd) != NEWLINE) /* strip comments */
			        ;
		tok = NEWLINE;
	}
	else if (c == GREATER || c == LESS || c == NOT
		 || c == BANG || c == CARET || c == EQUALS
		 || c == AND || c == OR)
		i = relate(lexstr, fd);
	if (i >= toksiz-1)
		synerr("token too long.");
	lexstr[i+1] = EOS;
	if (lexstr[0] == NEWLINE)
		linect[level] = linect[level] + 1;

#ifdef CRAY
/* cray cannot compare char and ints, since EOF is an int we check with feof */
	if (feof(fd)) tok = EOF;
#endif

	return(tok);
}

/*
 * lex - return lexical type of token
 *
 */
int lex(char *lexstr)
{

	int tok;

	for (tok = gnbtok(lexstr, MAXTOK);
	     tok == NEWLINE; tok = gnbtok(lexstr, MAXTOK))
		    ;
	if (tok == EOF || tok == SEMICOL || tok == LBRACE || tok == RBRACE)
		return(tok);
	if (tok == DIGIT)
		tok = LEXDIGITS;
	else if (equal(lexstr, sif) == YES)
		tok = vif[0];
	else if (equal(lexstr, selse) == YES)
		tok = velse[0];
	else if (equal(lexstr, swhile) == YES)
		tok = vwhile[0];
	else if (equal(lexstr, sdo) == YES)
		tok = vdo[0];
	else if (equal(lexstr, ssbreak) == YES)
		tok = vbreak[0];
	else if (equal(lexstr, snext) == YES)
		tok = vnext[0];
	else if (equal(lexstr, sfor) == YES)
		tok = vfor[0];
	else if (equal(lexstr, srept) == YES)
		tok = vrept[0];
	else if (equal(lexstr, suntil) == YES)
		tok = vuntil[0];
	else if (equal(lexstr, sswitch) == YES)
		tok = vswitch[0];
	else if (equal(lexstr, scase) == YES)
		tok = vcase[0];
	else if (equal(lexstr, sdefault) == YES)
		tok = vdefault[0];
	else if (equal(lexstr, sret) == YES)
		tok = vret[0];
	else if (equal(lexstr, sstr) == YES)
		tok = vstr[0];
	else
		tok = LEXOTHER;
	return(tok);
}

/*
 * ngetch - get a (possibly pushed back) character
 *
 */
char ngetch(char *c, FILE *fd)
{

	if (bp >= 0) {
		*c = buf[bp];
		bp--;
	}
	else
		*c = (char) getc(fd);

/*
 *					check for a continuation '_\n'
 *					also removes UNDERLINES from 
 *					variable names
 */
	while ( *c == UNDERLINE)
	{
		if (bp >= 0) {
			*c = buf[bp];
			bp--;
		}
		else
			*c = (char) getc(fd);

		if (*c != NEWLINE)
		{
			putbak(*c);
			*c=UNDERLINE;
			break;
		}
		else
		{
			while(*c == NEWLINE)
			{
				if (bp >= 0) {
					*c = buf[bp];
					bp--;
				}
				else
					*c = (char) getc(fd);
			}
		}
	}

	return(*c);
}

/*
 * pbstr - push string back onto input
 *
 */
void pbstr(const char *string)
{
	int i;

	for (i = strlen(string) - 1; i >= 0; i--)
		putbak(string[i]);
}

/*
 * putbak - push char back onto input
 *
 */
void putbak(char c)
{
	bp++;
	if (bp > BUFSIZE)
		baderr("too many characters pushed back.");
	buf[bp] = c;
}

/*
 * relate - convert relational shorthands into long form
 *
 */
int relate(char *token, FILE *fd)
{
	if (ngetch(&token[1], fd) != EQUALS) {
		putbak(token[1]);
		token[2] = LETT;
	}
	else
		token[2] = LETE;
	token[3] = PERIOD;
	token[4] = EOS;
	token[5] = EOS;	/* for .not. and .and. */
	if (token[0] == GREATER)
		token[1] = LETG;
	else if (token[0] == LESS)
		token[1] = LETL;
	else if (token[0] == NOT || token[0] == BANG || token[0] == CARET) {
		if (token[1] != EQUALS) {
			token[2] = LETO;
			token[3] = LETT;
			token[4] = PERIOD;
		}
		token[1] = LETN;
	}
	else if (token[0] == EQUALS) {
		if (token[1] != EQUALS) {
			token[2] = EOS;
			return(0);
		}
		token[1] = LETE;
		token[2] = LETQ;
	}
	else if (token[0] == AND) {		/* look for && or & */
	  if (ngetch(&token[1], fd) != AND) 
		                    putbak(token[1]);
		token[1] = LETA;
		token[2] = LETN;
		token[3] = LETD;
		token[4] = PERIOD;
	}
	else if (token[0] == OR) {
	  if (ngetch(&token[1], fd) != OR)	/* look for || or | */ 
		                    putbak(token[1]);
		token[1] = LETO;
		token[2] = LETR;
	}
	else   /* can't happen */
		token[1] = EOS;
	token[0] = PERIOD;
	return(strlen(token)-1);
}

/*
 * skpblk - skip blanks and tabs in file  fd
 *
 */
void skpblk(FILE *fd)
{
	char c;

	for (c = ngetch(&c, fd); c == BLANK || c == TAB; c = ngetch(&c, fd))
		;
	putbak(c);
}

/*
 * type - return LETTER, DIGIT or char; works with ascii alphabet
 *
 * to preserve the identity of EOF when casting back and forth between the
 * parameter value of type() and int, we use 'signed char' explicitly here,
 * so that the code also works on systems that use unsigned chars by default.
 *
 */
int type(signed char c)
{
	int t;

	if (c >= DIG0 && c <= DIG9)
		t = DIGIT;
	else if (c >= LETA && c <= LETZ)
		t = LETTER;
	else if (c >= BIGA && c <= BIGZ)
		t = LETTER;
	else
		t = c;
	return(t);
}

/*
 * C O D E  G E N E R A T I O N
 */

/*
 * brknxt - generate code for break n and next n; n = 1 is default
 */
void brknxt(int sp, int *lextyp, int *labval, int token)
{
	int i, n;
	char t, ptoken[MAXTOK];

	n = 0;
	t = gnbtok(ptoken, MAXTOK);
	if (alldig(ptoken) == YES) {     /* have break n or next n */
		i = 0;
		n = ctoi(ptoken, &i) - 1;
	}
	else if (t != SEMICOL)      /* default case */
		pbstr(ptoken);
	for (i = sp; i >= 0; i--)
		if (lextyp[i] == LEXWHILE || lextyp[i] == LEXDO
		    || lextyp[i] == LEXFOR || lextyp[i] == LEXREPEAT) {
			if (n > 0) {
				n--;
				continue;             /* seek proper level */
			}
			else if (token == LEXBREAK)
				outgo(labval[i]+1);
			else
				outgo(labval[i]);
/* original value
			xfer = YES;
*/
			xfer = NO;
			return;
		}
	if (token == LEXBREAK)
		synerr("illegal break.");
	else
		synerr("illegal next.");
	return;
}

/*
 * docode - generate code for beginning of do
 *
 */
void docode(int *lab)
{
	xfer = NO;
	outtab();
	outstr(sdo);
	*lab = labgen(2);
	outnum(*lab);
	eatup();
	outdon();
}

/*
 * dostat - generate code for end of do statement
 *
 */
void dostat(int lab)
{
	outcon(lab);
	outcon(lab+1);
}

/*
 * elseif - generate code for end of if before else
 *
 */
void elseif(int lab)
{

#ifdef F77
	outtab();
	outstr(selse);
	outdon();
#else
	outgo(lab+1);
	outcon(lab);
#endif /* F77 */
}

/*
 * forcod - beginning of for statement
 *
 */
void forcod(int *lab)
{
	char t, token[MAXTOK];
	int i, j, nlpar,tlab;

	tlab = *lab;
	tlab = labgen(3);
	outcon(0);
	if (gnbtok(token, MAXTOK) != LPAREN) {
		synerr("missing left paren.");
		return;
	}
	if (gnbtok(token, MAXTOK) != SEMICOL) {   /* real init clause */
		pbstr(token);
		outtab();
		eatup();
		outdon();
	}
	if (gnbtok(token, MAXTOK) == SEMICOL)   /* empty condition */
		outcon(tlab);
	else {   /* non-empty condition */
		pbstr(token);
		outnum(tlab);
		outtab();
		outstr(ifnot);
		outch(LPAREN);
		nlpar = 0;
		while (nlpar >= 0) {
			t = gettok(token, MAXTOK);
			if (t == SEMICOL)
				break;
			if (t == LPAREN)
				nlpar++;
			else if (t == RPAREN)
				nlpar--;
			if (t == (char)EOF) {
				pbstr(token);
				return;
			}
			if (t != NEWLINE && t != UNDERLINE)
				outstr(token);
		}
		outch(RPAREN);
		outch(RPAREN);
		outgo((tlab)+2);
		if (nlpar < 0)
			synerr("invalid for clause.");
	}
	fordep++;		/* stack reinit clause */
	j = 0;
	for (i = 1; i < fordep; i++)   /* find end *** should i = 1 ??? *** */
		j = j + strlen((char *) (&forstk[j])) + 1;
	forstk[j] = EOS;   /* null, in case no reinit */
	nlpar = 0;
	t = gnbtok(token, MAXTOK);
	pbstr(token);
	while (nlpar >= 0) {
		t = gettok(token, MAXTOK);
		if (t == LPAREN)
			nlpar++;
		else if (t == RPAREN)
			nlpar--;
		if (t == (char)EOF) {
			pbstr(token);
			break;
		}
		if (nlpar >= 0 && t != NEWLINE && t != UNDERLINE) {
			if ((j + (strlen((char *) (&token[0])))) >= MAXFORSTK)
				baderr("for clause too long.");
			scopy(token, 0, forstk, j);
			j = j + strlen((char *) (&token[0]));
		}
	}
	tlab++;   /* label for next's */
	*lab = tlab;
}

/*
 * fors - process end of for statement
 *
 */
void fors(int lab)
{
	int i, j;

	xfer = NO;
	outnum(lab);
	j = 0;
	for (i = 1; i < fordep; i++)
		j = j + strlen((char *) (&forstk[j])) + 1;
	if (strlen((char *) (&forstk[j])) > 0) {
		outtab();
		outstr(&forstk[j]);
		outdon();
	}
	outgo(lab-1);
	outcon(lab+1);
	fordep--;
}

/*
 * ifcode - generate initial code for if
 *
 */
void ifcode(int *lab)
{

	xfer = NO;
	*lab = labgen(2);
#ifdef F77
	ifthen();
#else
	ifgo(*lab);
#endif /* F77 */
}

#ifdef F77
/*
 * ifend - generate code for end of if
 *
 */
void ifend(void)
{
	outtab();
	outstr(sendif);
	outdon();
}
#endif /* F77 */

/*
 * ifgo - generate "if(.not.(...))goto lab"
 *
 */
void ifgo(int lab)
{

	outtab();      /* get to column 7 */
	outstr(ifnot);      /* " if(.not. " */
	balpar();      /* collect and output condition */
	outch(RPAREN);      /* " ) " */
	outgo(lab);         /* " goto lab " */
}

#ifdef F77
/*
 * ifthen - generate "if((...))then"
 *
 */
void ifthen(void)
{
	outtab();
	outstr(sif);
	balpar();
	outstr(sthen);
	outdon();
}
#endif /* F77 */

/*
 * labelc - output statement number
 *
 */
void labelc(const char *lexstr)
{
	xfer = NO;   /* can't suppress goto's now */
	if (strlen((char *) (&lexstr[0])) == 5)   /* warn about 23xxx labels */
		if (atoi((char*)lexstr) >= startlab)
			synerr("warning: possible label conflict.");
	outstr(lexstr);
	outtab();
}

/*
 * labgen - generate  n  consecutive labels, return first one
 *
 */
int labgen(int n)
{
	int i;

	i = label;
	label = label + n;
	return(i);
}

/*
 * otherc - output ordinary Fortran statement
 *
 */
void otherc(const char *lexstr)
{
	xfer = NO;
	outtab();
	outstr(lexstr);
	eatup();
	outdon();
}

/*
 * outch - put one char into output buffer
 *
 */
void outch(char c)
{
	int i;

	if (outp >= 72) {   /* continuation card */
		outdon();
		for (i = 0; i < 6; i++)
			outbuf[i] = BLANK;
		outbuf[5]='*';
		outp = 6;
	}
	outbuf[outp] = c;
	outp++;
}

/*
 * outcon - output "n   continue"
 *
 */
void outcon(int n)
{
	xfer = NO;
	if (n <= 0 && outp == 0)
		return;            /* don't need unlabeled continues */
	if (n > 0)
		outnum(n);
	outtab();
	outstr(contin);
	outdon();
}

/*
 * outdon - finish off an output line
 *
 */
void outdon(void)
{
	outbuf[outp] = NEWLINE;
	outbuf[outp+1] = EOS;
	printf("%s", outbuf);
	outp = 0;
}

/*
 * outcmnt - copy comment to output
 *
 */
void outcmnt(FILE *fd)
{
        char c;
        char comout[82];
        int comoutp=0;

        comoutp=1;
        comout[0]='C';
        while((c=ngetch(&c,fd)) != NEWLINE) {
           if (comoutp > 79) {
              comout[80]=NEWLINE;
              comout[81]=EOS;
              printf("%s",comout);
              comoutp=0;
              comout[comoutp]='C';
              comoutp++;
           }
           comout[comoutp]=c;
           comoutp++;
        }
        comout[comoutp]=NEWLINE;
        comout[comoutp+1]=EOS;
        printf("%s",comout);
}

/*
 * outasis - copy directly out
 *
 */
void outasis(FILE *fd)
{
	char c;
	while((c=ngetch(&c,fd)) != NEWLINE)
		outch(c);
	outdon();
}

/*
 * outgo - output "goto  n"
 *
 */
void outgo(int n)
{
	if (xfer == YES)
		return;
	outtab();
	outstr(rgoto);
	outnum(n);
	outdon();
}

/*
 * outnum - output decimal number
 *
 */
void outnum(int n)
{
	char chars[MAXCHARS];
	int i, m;

	m = abs(n);
	i = -1;
	do {
		i++;
		chars[i] = (m % 10) + DIG0;
		m = m / 10;
	}
	while (m > 0 && i < MAXCHARS);
	if (n < 0)
		outch(MINUS);
	for ( ; i >= 0; i--)
		outch(chars[i]);
}

/*
 * outstr - output string
 *
 */
void outstr(const char *str)
{
	int i;

	for (i=0; str[i] != EOS; i++)
		outch(str[i]);
}

/*
 * outtab - get past column 6
 *
 */
void outtab(void)
{
	while (outp < 6)
		outch(BLANK);
}

/*
 * repcod - generate code for beginning of repeat
 *
 */
void repcod(int *lab)
{
	int tlab;

	tlab = *lab;
	outcon(0);   /* in case there was a label */
	tlab = labgen(3);
	outcon(tlab);
	*lab = ++tlab;		/* label to go on next's */
}

/*
 * retcod - generate code for return
 *
 */
void retcod(void)
{
	char token[MAXTOK], t;

	t = gnbtok(token, MAXTOK);
	if (t != NEWLINE && t != SEMICOL && t != RBRACE) {
		pbstr(token);
		outtab();
		outstr(fcname);
		outch(EQUALS);
		eatup();
		outdon();
	}
	else if (t == RBRACE)
		pbstr(token);
	outtab();
	outstr(sret);
	outdon();
	xfer = YES;
}

/*
 * eosdef - return whether eos parameter is defined
 *
 */
int eosdef(void)
{
        char eosdfn[MAXDEF];
	if (look("eos",eosdfn) == YES)
		return YES;
	else if (look("EOS",eosdfn) == YES)
		return YES;
	else
		return NO;
}

#define SBUFSZ 10000
char savbuf[SBUFSZ];
int savlen = 0;

void savstr(const char *s)
{
	int i, size = SBUFSZ;
	int slen = strlen(s);
	int to_copy = size - savlen - 1;
	if (slen + 1 > (size - savlen))
		baderr("buffered string data too long.");
	for (i = 0; i < slen; ++i)
		savbuf[savlen+i] = s[i];
	savlen += strlen(s);
}

void saveos(void)
{
        char eosdfn[MAXDEF];
	if (look("eos",eosdfn) == YES) {
		savstr(eosdfn);
	} else if (look("EOS",eosdfn) == YES) {
		savstr(eosdfn);
	} else {
		savstr("eos");
	}
}

void savch(int ch)
{
        char foo[2];
	foo[0] = ch; 
	foo[1] = '\0';
	savstr(foo);
}

void savnum(int num)
{
        char foo[11];
	sprintf(foo,"%d",num);
	savstr(foo);
}

void outsav(void)
{
	int i;
	int tabbed = 0;
        for (i = 0; i < savlen; ++i) {
	        if (savbuf[i] == '\n') {
		        outdon();
			tabbed = 0;
		} else {
		        if (!tabbed) {
			        outtab();
				tabbed = 1;
			}
			outch(savbuf[i]);
		}
	}
	savlen = 0;
}

/*
 * strdcl - generate code for string declaration
 *
 */
void strdcl(void)
{
	char t, name[MAXNAME], init[MAXTOK], size[MAXTOK];
	int i, len;
	int quoted = 0, actlen = 0;

	t = gnbtok(name, MAXNAME);
	if (t != ALPHA)
		synerr("missing string name.");
	if (gnbtok(init, MAXTOK) != LPAREN) {  /* make size same as initial value */
		if (init[0] == SQUOTE || init[0] == DQUOTE) {
			/* minus two for quotes; plus one for eos */
			actlen = strlen(init) - 1;
			quoted = 1;
		} else {
			/* plus one for eos */
			actlen = strlen(init) + 1;
		}
		len = actlen;
		/* gtok should match up the quotes for us. */
	}
	else {	/* form is string name(size) init */
		t = gnbtok(size, MAXTOK);
		i = 0;
		len = ctoi(size, &i);
		if (size[i] != EOS)
			synerr("invalid string size.");
		/* we read the size, now consume the right parenthesis */
		if (gnbtok(init, MAXTOK) != RPAREN)
			synerr("missing right paren.");
		else {
			/* and finally read the initializer */
			t = gnbtok(init, MAXTOK);
		}
		if (init[0] == SQUOTE || init[0] == DQUOTE) {
			/* minus two for quotes; plus one for eos */
			actlen = strlen(init) - 1;
			quoted = 1;
		} else {
			/* plus one for eos */
			actlen = strlen(init) + 1;
		}
		if (len < 1) {
			synerr("specified string size too small.");
			len = 1;
		}
	}
	outtab();
	outstr(stelem);
	outch(BLANK);
	outstr(name);
	outch(LPAREN);
	outnum(len);
	outch(RPAREN);
	outdon();
	outtab();
	savstr(dat);
	if (quoted) {
		init[actlen] = EOS; /* delete the ending quote */
		scopy(init, 1, init, 0); /* delete the starting quote */
	}
	/* make sure user-provided size will result in a reasonable data stmt */
	if (len > actlen) {
		len = actlen; /* only initialize beginning of string */
	}
	else if (len < actlen) {
		init[len-1] = EOS; /* truncate string to user-specified size */
	}
	for (i = 1; i <= len; i++) {	/* put out variable names */
		savstr(name);
		savch(LPAREN);
		savnum(i);
		savch(RPAREN);
		if (i < len)
			savch(COMMA);
		else
			savch(SLASH);
	}
	for (i = 0; init[i] != EOS; i++) {	/* put out init */
		savnum(init[i]);
		savch(COMMA);
	}
	if (eosdef() == NO)   /* EOS undefined */
		synerr("warning: string directive used when EOS undefined.");
	saveos();
	savch(SLASH);
	savch('\n');
	/* Peek ahead to see whether we should emit the data statements now.
	 * We don't do it yet if the next token is LEXSTRING or 
	 * if the next two tokens are SEMICOL LEXSTRING. 
	 * This is to avoid emitting code for a series of string declarations
	 * all in a row which would trip over a pedantic error raised by f2c,
	 * which insists that all variable declarations must come before any
	 * other statements, including data statements. */
	i = lex(init);
	if (i != LEXSTRING) {
		char temp[MAXTOK];
		if (i == SEMICOL) {
			i = lex(temp);
			if (i != LEXSTRING) {
				outsav();
				outdon();
			}
			pbstr(temp);
		} else {
			outsav();
			outdon();
		}
	}
	pbstr(init);
}

/*
 * unstak - unstack at end of statement
 *
 */
void unstak(int *sp, const int *lextyp, const int *labval, char token)
{
	int tp;

	tp = *sp;
	for ( ; tp > 0; tp--) {
		if (lextyp[tp] == LBRACE)
			break;
		if (lextyp[tp] == LEXSWITCH)
			break;
		if (lextyp[tp] == LEXIF && token == LEXELSE)
			break;
		if (lextyp[tp] == LEXIF)
#ifdef F77
			ifend();
#else
			outcon(labval[tp]);
#endif /* F77 */
		else if (lextyp[tp] == LEXELSE) {
			if (*sp > 1)
				tp--;
#ifdef F77
			ifend();
#else
			outcon(labval[tp]+1);
#endif /* F77 */
		}
		else if (lextyp[tp] == LEXDO)
			dostat(labval[tp]);
		else if (lextyp[tp] == LEXWHILE)
			whiles(labval[tp]);
		else if (lextyp[tp] == LEXFOR)
			fors(labval[tp]);
		else if (lextyp[tp] == LEXREPEAT)
			untils(labval[tp], token);
	}
	*sp = tp;
}

/*
 * untils - generate code for until or end of repeat
 *
 */
void untils(int lab, int token)
{
	char ptoken[MAXTOK];

	xfer = NO;
	outnum(lab);
	if (token == LEXUNTIL) {
		lex(ptoken);
		ifgo(lab-1);
	}
	else
		outgo(lab-1);
	outcon(lab+1);
}

/*
 * whilec - generate code for beginning of while
 *
 */
void whilec(int *lab)
{
	int tlab;

	tlab = *lab;
	outcon(0);         /* unlabeled continue, in case there was a label */
	tlab = labgen(2);
	outnum(tlab);
#ifdef F77
	ifthen();
#else
	ifgo(tlab+1);
#endif /* F77 */
	*lab = tlab;
}

/*
 * whiles - generate code for end of while
 *
 */
void whiles(int lab)
{

	outgo(lab);
#ifdef F77
	ifend();
#endif /* F77 */
	outcon(lab+1);
}

/*
 * E R R O R  M E S S A G E S
 */

/*
 *  baderr - print error message, then die
 */
void baderr(const char *msg)
{
	synerr(msg);
	exit(1);
}

/*
 * error - print error message with one parameter, then die
 */
void error(const char *msg, const char *s)
{
	fprintf(stderr, msg, s);
	exit(1);
}

/*
 * synerr - report Ratfor syntax error
 */
void synerr(const char *msg)
{
	char lc[MAXCHARS];
	int i;

	fputs(errmsg, stderr);
	if (level >= 0)
		i = level;
	else
		i = 0;   /* for EOF errors */
	itoc(linect[i], lc, MAXCHARS);
	fputs((char*)lc, stderr);
	for (i = fnamp - 1; i > 1; i = i - 1)
		if (fnames[i-1] == EOS) {   /* print file name */
			fputs(in, stderr);
			fputs((char*)&fnames[i], stderr);
			break;
		}
	fprintf(stderr,": \n      %s\n",msg);
}


/*
 * U T I L I T Y  R O U T I N E S
 */

/*
 * ctoi - convert string at in[i] to int, increment i
 */
int ctoi(char *in_, int *i)
{
	int k, j;

	j = *i;
	while (in_[j] == BLANK || in_[j] == TAB)
		j++;
	for (k = 0; in_[j] != EOS; j++) {
		if (in_[j] < DIG0 || in_[j] > DIG9)
			break;
		k = 10 * k + in_[j] - DIG0;
	}
	*i = j;
	return(k);
}

/*
 * fold - convert alphabetic token to single case
 *
 */
void fold(char *token)
{

	int i;

	/* WARNING - this routine depends heavily on the */
	/* fact that letters have been mapped into internal */
	/* right-adjusted ascii. god help you if you */
	/* have subverted this mechanism. */

	for (i = 0; token[i] != EOS; i++)
		if (token[i] >= BIGA && token[i] <= BIGZ)
			token[i] = token[i] - BIGA + LETA;
}

/*
 * equal - compare str1 to str2; return YES if equal, NO if not
 *
 */
int equal(const char *str1, const char *str2)
{
	int i;

	for (i = 0; str1[i] == str2[i]; i++)
		if (str1[i] == EOS)
			return(YES);
	return(NO);
}

/*
 * scopy - copy string at from[i] to to[j]
 *
 */
void scopy(const char *from, int i, char *to, int j)
{
	int k1, k2;

	k2 = j;
	for (k1 = i; from[k1] != EOS; k1++) {
		to[k2] = from[k1];
		k2++;
	}
	to[k2] = EOS;
}

/*
 * look - look-up a definition
 *
 */
int look(const char *name,char *defn)
{
	struct hashlist *p;

	if ((p = lookup(name)) == NULL)
		return(NO);
	(void) strcpy((char *) (&defn[0]),(char *) (&((p->def)[0])));
	return(YES);
}

/*
 * itoc - special version of itoa
 */
int itoc(int n, char *str, int size)
{
	int i,j,k,sign;
	char c;

	if ((sign = n) < 0)
		n = -n;
	i = 0;
	do {
		str[i++] = n % 10 + '0';
	}
	while ((n /= 10) > 0 && i < size-2);
	if (sign < 0 && i < size-1)
		str[i++] = '-';
	str[i] = EOS;
	/*
	 * reverse the string and plug it back in
	 */
	for (j = 0, k = strlen((char *) (&str[0])) - 1; j < k; j++, k--) {
		c = str[j];
		str[j] = str[k];
		str[k] = c;
	}
	return(i-1);
}

/*
 * cascod - generate code for case or default label
 *
 */
void cascod (int lab, int token)
{
	int t, l, lb, ub, i, j;
	char scrtok[MAXTOK];

	if (swtop <= 0) {
		synerr ("illegal case or default.");
		return;
	}
	outgo(lab + 1);		/* # terminate previous case */
	xfer = YES;
	l = labgen(1);
	if (token == LEXCASE) { 	/* # case n[,n]... : ... */
		while (caslab (&lb, &t) != EOF) {
			ub = lb;
			if (t == MINUS)
				caslab (&ub, &t);
			if (lb > ub) {
				synerr ("illegal range in case label.");
				ub = lb;
			}
			if (swlast + 3 > MAXSWITCH)
				baderr ("switch table overflow.");
			for (i = swtop + 3; i < swlast; i = i + 3)
				if (lb <= swstak[i])
					break;
				else if (lb <= swstak[i+1])
					synerr ("duplicate case label.");
			if (i < swlast && ub >= swstak[i])
				synerr ("duplicate case label.");
			for (j = swlast; j > i; j--)   	/* # insert new entry */
				swstak[j+2] = swstak[j-1];
			swstak[i] = lb;
			swstak[i + 1] = ub;
			swstak[i + 2] = l;
			swstak[swtop + 1] = swstak[swtop + 1]  +  1;
			swlast = swlast + 3;
			if (t == COLON)
				break;
			else if (t != COMMA)
				synerr ("illegal case syntax.");
		}
	}
	else {   					/* # default : ... */
		t = gnbtok (scrtok, MAXTOK);
		if (swstak[swtop + 2] != 0)
			baderr ("multiple defaults in switch statement.");
		else
			swstak[swtop + 2] = l;
	}

	if (t == EOF)
		synerr ("unexpected EOF.");
	else if (t != COLON)
		baderr ("missing colon in case or default label.");

	xfer = NO;
	outcon (l);
}

/*
 * caslab - get one case label
 *
 */
int caslab (int *n, int *t)
{
	char tok[MAXTOK];
	int i, s;

	*t = gnbtok (tok, MAXTOK);
	while (*t == NEWLINE)
		*t = gnbtok (tok, MAXTOK);
	if (*t == EOF)
		return (*t);
	if (*t == MINUS)
		s = -1;
	else
		s = 1;
	if (*t == MINUS || *t == PLUS)
		*t = gnbtok (tok, MAXTOK);
	if (*t != DIGIT) {
		synerr ("invalid case label.");
		*n = 0;
	} else {
		i = 0;
		*n = s * ctoi (tok, &i);
	}
	*t = gnbtok (tok, MAXTOK);
	while (*t == NEWLINE)
		*t = gnbtok (tok, MAXTOK);
	return 0;
}

/*
 * swcode - generate code for switch stmt.
 *
 */
void swcode (int *lab)
{
	char scrtok[MAXTOK];

	*lab = labgen (2);
	if (swlast + 3 > MAXSWITCH)
		baderr ("switch table overflow.");
	swstak[swlast] = swtop;
	swstak[swlast + 1] = 0;
	swstak[swlast + 2] = 0;
	swtop = swlast;
	swlast = swlast + 3;
	xfer = NO;
	outtab();  	/* # Innn=(e) */
	swvar(*lab);
	outch(EQUALS);
	balpar();
	outdon();
	outgo(*lab); 	/* # goto L */
	xfer = YES;
	while (gnbtok (scrtok, MAXTOK) == NEWLINE)
		;
	if (scrtok[0] != LBRACE) {
		synerr ("missing left brace in switch statement.");
		pbstr (scrtok);
	}
}

/*
 * swend  - finish off switch statement; generate dispatch code
 *
 */
void swend(int lab)
{
	int n, i;
#ifdef OUTPUT_BRANCH_TABLE
	int lb, ub;
	int j;

static	const char *slt   	= ".lt.1.or.";
static	const char *sgt   	= ".gt.";
static	const char *sgoto 	= "goto (";
#endif
static	const char *sifp   	= "if (";
static	const char *seq   	= ".eq.";
static	const char *sge   	= ".ge.";
static	const char *sle   	= ".le.";
static	const char *sand  	= ".and.";

#ifdef OUTPUT_BRANCH_TABLE
	lb = swstak[swtop + 3];
	ub = swstak[swlast - 2];
#endif
	n = swstak[swtop + 1];
	outgo(lab + 1); 			/* # terminate last case */
	if (swstak[swtop + 2] == 0)
		swstak[swtop + 2] = lab + 1;	/* # default default label */
	xfer = NO;
	outcon (lab);  			/*  L   continue */
	/* output branch table */
#ifdef OUTPUT_BRANCH_TABLE
	if (n >= CUTOFF && ub - lb < DENSITY * n) {
		if (lb != 0) {  		   /* L  Innn=Innn-lb */
			outtab();
			swvar  (lab);
			outch (EQUALS);
			swvar  (lab);
			if (lb < 0)
				outch (PLUS);
			outnum (-lb + 1);
			outdon();
		}
		outtab();  /*  if (Innn.lt.1.or.Innn.gt.ub-lb+1)goto default */
		outstr (sifp);
		swvar  (lab);
		outstr (slt);
		swvar  (lab);
		outstr (sgt);
		outnum (ub - lb + 1);
		outch (RPAREN);
		outgo (swstak[swtop + 2]);
		outtab();
		outstr (sgoto);		/* goto ... */
		j = lb;
		for (i = swtop + 3; i < swlast; i = i + 3) {
			 * # fill in vacancies * 
			for ( ; j < swstak[i]; j++) {
				outnum(swstak[swtop + 2]);
				outch(COMMA);
			}
			for (j = swstak[i + 1] - swstak[i]; j >= 0; j--)
				outnum(swstak[i + 2]);	 /* # fill in range */
			j = swstak[i + 1] + 1;
			if (i < swlast - 3)
				outch(COMMA);
		}
		outch(RPAREN);
		outch(COMMA);
		swvar(lab);
		outdon();
	}
	else if (n > 0) { 		 /* # output linear search form */
#endif /* OUTPUT_BRANCH_TABLE */
	if (n > 0) { 		/* # output linear search form */
		for (i = swtop + 3; i < swlast; i = i + 3) {
			outtab();		/* # if (Innn */
			outstr (sifp);
			swvar  (lab);
			if (swstak[i] == swstak[i+1]) {
				outstr (seq); 	/* #   .eq....*/
				outnum (swstak[i]);
			}
			else {
				outstr (sge);	/* #   .ge.lb.and.Innn.le.ub */
				outnum (swstak[i]);
				outstr (sand);
				swvar  (lab);
				outstr (sle);
				outnum (swstak[i + 1]);
			}
			outch (RPAREN);		/* #    ) goto ... */
			outgo (swstak[i + 2]);
		}
		if (lab + 1 != swstak[swtop + 2])
			outgo (swstak[swtop + 2]);
	}
	outcon (lab + 1);   			/* # L+1  continue */
	swlast = swtop;				/* # pop switch stack */
	swtop = swstak[swtop];
}

/*
 * swvar  - output switch variable Innn, where nnn = lab
 */
void swvar (int lab)
{
	outch ('I');
	outnum (lab);
}

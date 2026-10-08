static char *sccsid = "@(#)usq.c        1.7u (UCF) 82/12/15";
/*
 * 	usq.c - CP/M compatible file unsqueezer utility
 *
 *	compile as follows:
 *	cc [-DVAX] -O usq.c -o usq
 *	   (define VAX only if running on VAX)
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define ERROR (-1)
#define PATHLEN	312	/* Number of characters allowed in pathname */
#define OK 0

#define RECOGNIZE 0xFF76	/* unlikely pattern */
#define DLE 0x90		/* repeat byte flag */
#define SPEOF 256		/* special endfile token */
#define NUMVALS 257		/* 256 data values plus SPEOF*/
#define LARGE 30000

#ifdef VAX   /* then we don't want 32 bit integers */

typedef short INT;
typedef unsigned short UNSIGNED;

#else   /*  16 bit machines  */

typedef int INT;
typedef unsigned UNSIGNED;

#endif

struct _sqleaf {		/* Decoding tree */
	INT _children[2];	/* left, right */
};
struct _sqleaf Dnode[NUMVALS - 1];


INT Bpos;		/* last bit position read */
INT Curin;		/* last byte value read */
INT Repct;		/* Number of times to return value */
INT Value;		/* current byte value or EOF */

INT MakeLCPathname=TRUE;	/* translate pathname to lc if all caps */
INT Nlmode=FALSE;		/* zap cr's if true */
INT Inbackground = FALSE;

INT getcr(), getuhuff(), portgetw();

main(argc, argv)
char *argv[];
{
	register char *cp;
	register INT npats=0;
	char **patts;
	INT n, errorstat;

	if (signal(SIGINT, SIG_IGN)==SIG_IGN)
		Inbackground++;
	else
		signal(SIGINT, SIG_DFL);
#ifdef SIGHUP	/* not in the Aztec CP/M-86 signal.h */
	signal(SIGHUP, SIG_IGN);
#endif
	errorstat=0;
	if(argc<2)
		goto usage;
	while (--argc) {
		cp = *++argv;
		if(*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				case 'n':
				case 'N':
					Nlmode=TRUE; break;
				case 'u':
				case 'U':
					MakeLCPathname=FALSE; break;
				default:
					goto usage;
				}
			}
		}
		else if( !npats && argc>0) {
			if(argv[0][0]) {
				npats=argc;
				patts=argv;
			}
		}
	}
	if(npats < 1) {
usage:
		fprintf(stderr,"Usage: usq [-nu] file ...\n");
		fprintf(stderr,"\t-n Nlmode: remove carriage returns\n");
		fprintf(stderr,"\t-u preserve Uppercase pathnames\n");
		exit(1);
	}
	for(n=0; n<npats; ++n)
		errorstat |= squeeze(patts[n]);
	exit(errorstat != 0);
}

/*
	The following code is primarily from typesq.c and utr.c.  Typesq
is a modification of USQ by Dick Greenlaw.  Those modifications (usq
to typesq) were made by Bob Mathias, I am responsible for the butchery
done to make it work with cat.

*/

FILE *in, *out;
squeeze(fname)
char *fname;
{
	register INT i, c;
	register char *p;
	register INT numnodes;			/* size of decoding tree */
	register UNSIGNED crc;
	UNSIGNED filecrc;
	char origname[PATHLEN];		/* Original file name without drive */

	init_cr(); init_huff(); crc=0;

	if((in=fopen( fname, "r"))==NULL) {
		fprintf(stderr, "usq: can't open %s\n", fname);
		return ERROR;
	}
	if(portgetw(in) != (INT) RECOGNIZE) {/* Process header */
		fprintf(stderr, "usq: %s is not a SQueezed file\n", fname);
		return(ERROR);
	}
	filecrc = (UNSIGNED) portgetw(in);	/* checksum */
	p = origname;				/* Get original file name */
	do {					/* send it to array */
		*p = getc(in);
	} while(*p++ != '\0');

	numnodes = portgetw(in);
	if(numnodes < 0 || numnodes >= NUMVALS) {
		fprintf(stderr, "usq: %s has invalid decode tree\n", fname);
		fclose(in);
		return(ERROR);
	}
	/* Initialize for possible empty tree (SPEOF only) */
	Dnode[0]._children[0] = -(SPEOF + 1);
	Dnode[0]._children[1] = -(SPEOF + 1);

	for(i = 0; i < numnodes; ++i) {	/* Get decoding tree from file */
		Dnode[i]._children[0] = portgetw(in);
		Dnode[i]._children[1] = portgetw(in);
	}
	/* Get translated output bytes and write file */
	if(MakeLCPathname && !IsAnyLower(origname))
		uncaps(origname);
	for(p=origname; *p; ++p)		/* change / to _ */
		if( *p == '/')
			*p = '_';
	if (!Inbackground)
		fprintf(stderr, "usq: %s -> %s\n",fname,origname);
	if((out=fopen(origname, "w"))==NULL) {
		fprintf(stderr, "usq: can't create %s\n", origname);
	}
	while ((c = getcr()) != EOF) {
		crc += (UNSIGNED) c;
		if ( c == '\r' && Nlmode)
			continue;
		putc(c, out);
	}
	fclose(in);
	fflush(out);
	fclose(out);
	if( crc != filecrc ) {
		fprintf(stderr, "usq: bad checksum in %s\n", fname);
		fflush(stdout);
		return(ERROR);
	}
	return(OK);
}
/*** from utr.c - */
/* initialize decoding functions */

init_cr()
{
	Repct = 0;
}

init_huff()
{
	Bpos = 99;	/* force initial read */
}

/* Get bytes with decoding - this decodes repetition,
 * calls getuhuff to decode file stream into byte
 * level code with only repetition encoding.
 *
 * The code is simple passing through of bytes except
 * that DLE is encoded as DLE-zero and other values
 * repeated more than twice are encoded as value-DLE-count.
 */

INT
getcr()
{
	register INT c;

	if(Repct > 0) {
		/* Expanding a repeated char */
		--Repct;
		return(Value);
	} else {
		/* Nothing unusual */
		if((c = getuhuff()) != DLE) {
			/* It's not the special delimiter */
			Value = c;
			if(Value == EOF)
				Repct = LARGE;
			return(Value);
		} else {
			/* Special token */
			if((Repct = getuhuff()) == 0)
				/* DLE, zero represents DLE */
				return(DLE);
			else {
				/* Begin expanding repetition */
				Repct -= 2;	/* 2nd time */
				return(Value);
			}
		}
	}
}
/* Decode file stream into a byte level code with only
 * repetition encoding remaining.
 */

INT
getuhuff()
{
	register INT i;

	/* Follow bit stream in tree to a leaf*/
	i = 0;	/* Start at root of tree */
	do {
		if(++Bpos > 7) {
			if((Curin = getc(in)) == ERROR)
				return(ERROR);
			Bpos = 0;
			/* move a level deeper in tree */
			i = Dnode[i]._children[1 & Curin];
		} else
			i = Dnode[i]._children[1 & (Curin >>= 1)];
	} while(i >= 0);

	/* Decode fake node index to original data value */
	i = -(i + 1);
	/* Decode special endfile token to normal EOF */
	i = (i == SPEOF) ? EOF : i;
	return(i);
}
/*
 * Machine independent getw which always gets bytes in the same order
 *  as the CP/M version of SQ wrote them
 */
INT
portgetw(f)
FILE *f;
{
	register INT c;

	c = getc(f) & 0377;
	return(c | (getc(f) << 8));
}


/* make string s lower case */
uncaps(s)
char *s;
{
	for( ; *s; ++s)
		if(isupper(*s))
			*s = tolower(*s);
}


/*
 * IsAnyLower returns TRUE if string s has lower case letters.
 */
IsAnyLower(s)
char *s;
{
	for( ; *s; ++s)
		if (islower(*s))
			return(TRUE);
	return(FALSE);
}

/*
*********************************************************
*							*
*		   LU for CP/M-86			*
*							*
*********************************************************

This version of LU is mostly just the UNIX LU program
"lar" with a very few modifications to take into
account differences between UNIX and CP/M system
conventions. It is intended for compilation with the
Digital Research C compiler (which really is VERY
syntax compatible with UNIX 7 C code). I would also
think that an PCDOS version of LU would be easy to
generate from this code using the PCDOS version of
the DR C compiler.  

This is realy just the first pass through "lar". I hope to
have versions with added features soon. However this version
is perfectly adequate for the basic manipulation of
LBR libraries under in CP/M-86 and Concurrent PCDOS
environments.

				Bill Bolton,
				Software Tools RCPM,
				Brisbane, Australia


VERSION LIST, most recent version first

08/Oct/26 Version 1.1 for Aztec C86, with fixes and features taken
	  from the MS-DOS ports of "lar" (lar 1.3 by T. Bonfield and
	  R. McVay, lar 2.2 by P.H. Mack):
	  - wildcards (* and ?) in member names; for u/a they are
	    expanded against the disk directory
	  - the library type defaults to .LBR
	  - a (add) and l (list) as LU-style aliases for u and t
	  - a leading - on the key is accepted
	  - reports whether each member is added or replaced
	  - a drive prefix is no longer stored in member names
	  - asking for n slots gives n members plus the directory
	  - reorganize no longer fails when the new library is
	    exactly full
	  - file I/O does not depend on seeking to the end of a file,
	    which CP/M cannot do reliably for a file being written

09/Aug/84 Minor modifications to Stephen Hemmingers UNIX
          "lar" program. DRC always returns command line
	  arguments in lower case (how's that for counter
	  intuitive, at least to someone who uses CP/M-86
	  assembler too!) so that had to be taken into
	  account. Version 1.0  Bill Bolton	

Original UNIX version documentation follows

 - /bin/ncc -O lar.c -o lar
From linus!sch Tue Jul 26 08:07:37 1983
Subject: CP/M Lu library maintainer

When transfering files to my personal computer, I often want to transfer
several files at once using the Umodem program.  To do this I wrote
the following small program to combine files for the CP/M LU program.

No special treatment necessary, just:
	cc -O lar.c -o lar
to make it.

-- 
Stephen Hemminger,  Mitre Corp. Bedford MA 
	{allegra,genrad,ihnp4, utzoo}!linus!sch	(UUCP)
	linus!sch@mitre-bedford			(ARPA)
----------------- lar.c ----------------------
*/

/*
 * Lar - LUU format library file maintainer
 * by Stephen C. Hemminger
 *	linus!sch	or	sch@Mitre-Bedford
 *
 *  Usage: lar key library [files] ...
 *
 *  Key functions are:
 *	u - Update, add files to library
 *	t - Table of contents
 *	e - Extract files from library
 *	p - Print files in library
 *	d - Delete files in library
 *	r - Reorginize library
 *  Other keys:
 *	v - Verbose
 *
 *  This program is public domain software, no warranty intended or
 *  implied.
 *
 *  DESCRPTION
 *     Lar is a Unix program to manipulate CP/M LU format libraries.
 *     The original CP/M library program LU is the product
 *     of Gary P. Novosielski. The primary use of lar is to combine several
 *     files together for upload/download to a personal computer.
 *
 *  PORTABILITY
 *     The code is modeled after the Software tools archive program,
 *     and is setup for Version 7 Unix.  It does not make any assumptions
 *     about byte ordering, explict and's and shift's are used.
 *     If you have a dumber C compiler, you may have to recode new features
 *     like structure assignment, typedef's and enumerated types.
 *
 *  BUGS/MISFEATURES
 *     The biggest problem is text files, the programs tries to detect
 *     text files vs. binaries by checking for non-Ascii (8th bit set) chars.
 *     If the file is text then it will throw away Control-Z chars which
 *     CP/M puts on the end.  All files in library are padded with Control-Z
 *     at the end to the CP/M sector size if necessary.
 *
 *     No effort is made to handle the difference between CP/M and Unix
 *     end of line chars.  CP/M uses Cr/Lf and Unix just uses Lf.
 *     The solution is just to use the Unix command sed when necessary.
 *
 *  * Unix is a trademark of Bell Labs.
 *  ** CP/M is a trademark of Digital Research.
 */

#include <stdio.h>
#include <ctype.h>

#ifdef AZTEC
#define fopenb	fopen	/* Aztec getc/putc/fread/fwrite are already binary */
#define rewind(f)	fseek(f, 0L, 0)
#endif

#define ACTIVE	00
#define UNUSED	0xff
#define DELETED 0xfe
#define CTRLZ	0x1a

#define MAXFILES 256
#define SECTOR	 128
#define DSIZE	( sizeof(struct ludir) )
#define SLOTS_SEC (SECTOR/DSIZE)
#define equal(s1, s2) ( strcmp(s1,s2) == 0 )

#define VERS 1		/* major revision number */
#define REV 1		/* minor revision number */

/* if you don't have void type just define as blank */

#define VOID	(void)

/* if no enum's then define false as 0 and true as 1 and bool as int */
/* typedef enum {false=0, true=1} bool; */

#define false 0
#define true 1
#define bool int

/* Globals */

char   *fname[MAXFILES];
bool ftouched[MAXFILES];

typedef struct {
    char   lobyte;
    char   hibyte;
} word;

/* convert word to int */

#define wtoi(w) ( ((w.hibyte & 0xff)<<8) + (w.lobyte & 0xff) )
#define itow(dst,src)	dst.hibyte = (src & 0xff00) >> 8;\
				dst.lobyte = src & 0xff;

struct ludir {			/* Internal library ldir structure */
    char    l_stat;		/*  status of file */
    char    l_name[8];		/*  name */
    char    l_ext[3];		/*  extension */
    word    l_off;		/*  offset in library */
    word    l_len;		/*  lengty of file */
    char    l_fill[16];		/*  pad to 32 bytes */
} ldir[MAXFILES];

int     errcnt, nfiles, nslots;
int	endsec;			/* first free sector at the end of the library */
bool	verbose = false;
char	*cmdname;

char   *getname(), *sprintf(), *nopath(), *index(), *strcat(), *fgets();
int	update(), reorg(), table(), extract(), print(), delete();



main (argc, argv)
int	argc;
char  **argv;
{
    register char *flagp;
    char   *aname;			/* name of library file */
    static char libname[20];
    int	   (*function)() = NULL;	/* function to do on library */
/* set the function to be performed, but detect conflicts */
#define setfunc(val)	if(function != NULL) conflict(); else function = val

#ifdef UNIX
    cmdname = argv[0];
#else
    cmdname = "LUU";

    printf ("\n%s - library maintenance utility, CP/M-86 version %d.%d\n\n",
     cmdname,VERS,REV);
#endif /* UNIX */

    if (argc < 3)
	help ();

#ifdef UNIX
    aname = argv[2];
#else
    aname = libname;
    ucase (aname,argv[2]);	/* get library name */
    if (index (nopath (aname), '.') == NULL)
	VOID strcat (aname, ".LBR");
#endif /* UNIX */

    filenames (argc, argv);

    for(flagp = argv[1]; *flagp; flagp++)
	switch (isupper(*flagp) ? *flagp - 'A' + 'a' : *flagp) {
	case '-':
		break;
	case 'u': 
	case 'a':
	    setfunc(update);
	    break;
	case 't': 
	case 'l':
	    setfunc(table);
	    break;
	case 'e': 
	    setfunc(extract);
	    break;
	case 'p': 
	    setfunc(print);
	    break;
	case 'd': 
	    setfunc(delete);
	    break;
	case 'r': 
	    setfunc(reorg);
	    break;
	case 'v':
	    verbose = true;
	    break;
	default: 
	    help ();
    }

    if(function == NULL) {
	fprintf(stderr,"No function key letter specified\n");
	help();
    }

    (*function)(aname);
}

/* print error message and exit */
help () {
    fprintf (stderr, "Usage: %s [-]{utepdr}[v] library[.LBR] [files] ...\n", cmdname);
    fprintf (stderr, "Functions are:\n\tu - Update, add files to library (or a)\n");
    fprintf (stderr, "\tt - Table of contents (or l)\n");
    fprintf (stderr, "\te - Extract files from library\n");
    fprintf (stderr, "\tp - Print files in library\n");
    fprintf (stderr, "\td - Delete files in library\n");
    fprintf (stderr, "\tr - Reorginize library\n");

    fprintf (stderr, "Flags are:\n\tv - Verbose\n");
    fprintf (stderr, "Files may use the * and ? wildcards.\n");
    exit (1);
}

conflict() {
   fprintf(stderr,"Conficting keys\n");
   help();
}

error (str)
char   *str;
{
    fprintf (stderr, "%s: %s\n", cmdname, str);
    exit (1);
}

cant (name)
char   *name;
{
#ifdef AZTEC
    fprintf (stderr, "%s: can't open\n", name);
#else
    extern int  errno;
    extern char *sys_errlist[];

    fprintf (stderr, "%s: %s\n", name, sys_errlist[errno]);
#endif
    exit (1);
}

/* Get file names, check for dups, and initialize */
filenames (ac, av)
char  **av;
{
    register int    i, j;
    register char *cp, *dp;

    errcnt = 0;
    for (i = 0; i < ac - 3; i++) {
#ifdef UNIX
	fname[i] = av[i + 3];
#else
	fname[i] = av[i + 3];
	ucase (fname[i],fname[i]);
#endif /* UNIX */

	ftouched[i] = false;
	if (i == MAXFILES)
	    error ("Too many file names.");
    }
    fname[i] = NULL;
    nfiles = i;
    for (i = 0; i < nfiles; i++)
	for (j = i + 1; j < nfiles; j++)
	    if (equal (fname[i], fname[j])) {
		fprintf (stderr, "%s", fname[i]);
		error (": duplicate file name");
	    }
}

table (lib)
char   *lib;
{
    FILE   *lfd;
    register int    i, total;
    int active = 0, unused = 0, deleted = 0;
    char *uname;

    if ((lfd = fopenb (lib, "r")) == NULL)
	cant (lib);

    getdir (lfd);
    total = wtoi(ldir[0].l_len);
    if(verbose) {
 	printf("Name          Index Length\n");
	printf("Directory           %4d\n", total);
    }

    for (i = 1; i < nslots; i++)
	switch(ldir[i].l_stat & 0xff) {
	case ACTIVE:
		active++;
		uname = getname(ldir[i].l_name, ldir[i].l_ext);
		if (filarg (uname))
		    if(verbose)
			printf ("%-12s   %4d %4d\n", uname,
			    wtoi (ldir[i].l_off), wtoi (ldir[i].l_len));
		    else
			printf ("%s\n", uname);
		total += wtoi(ldir[i].l_len);
		break;
	case UNUSED:
		unused++;
		break;
	default:
		deleted++;
	}
    if(verbose) {
	printf("--------------------------\n");
	printf("Total sectors       %4d\n", total);
	printf("\nLibrary %s has %d slots, %d deleted, %d active, %d unused\n",
		lib, nslots, deleted, active, unused);
    }

    VOID fclose (lfd);
    not_found ();
}

getdir (f)
FILE *f;
{

    rewind(f);

    if (fread ((char *) & ldir[0], DSIZE, 1, f) != 1)
	error ("No directory\n");

    nslots = wtoi (ldir[0].l_len) * SLOTS_SEC;

    if (fread ((char *) & ldir[1], DSIZE, nslots - 1, f) != nslots - 1)
	error ("Can't read directory - is it a library?");
}

/*
 * End of the library in sectors, from the directory. Seeking to the end of
 * the file is not reliable on CP/M: the size of a file that is open for
 * writing is not known until it is closed.
 */
libend ()
{
    register int    i, n, e;

    for (e = i = 0; i < nslots; i++)
	if (ldir[i].l_stat == ACTIVE) {
	    n = wtoi (ldir[i].l_off) + wtoi (ldir[i].l_len);
	    if (n > e)
		e = n;
	}
    return e;
}

putdir (f)
FILE *f;
{

    rewind(f);
    if (fwrite ((char *) ldir, DSIZE, nslots, f) != nslots)
	error ("Can't write directory - library may be botched");
}

initdir (f)
FILE *f;
{
    register int    i;
    int     numsecs;
    char    line[80];
    static struct ludir blankentry = {
	UNUSED,
	"        ",
	"   "
    };

    for (;;) {
	printf ("Number of slots to allocate: ");
	if (fgets (line, 80, stdin) == NULL)
	    error ("EOF when reading input");
	nslots = atoi (line);
	if (nslots < 1)
	    printf ("Must have at least one!\n");
	else if (nslots > MAXFILES)
	    printf ("Too many slots\n");
	else
	    break;
    }

    numsecs = (nslots + 1 + SLOTS_SEC - 1) / SLOTS_SEC;	/* + directory */
    nslots = numsecs * SLOTS_SEC;

    for (i = 0; i < nslots; i++)
	ldir[i] = blankentry;
    ldir[0].l_stat = ACTIVE;
    itow (ldir[0].l_len, numsecs);

    putdir (f);
}

ucase (ustring,lstring)

char *ustring, *lstring;

{
	register char *cp, *dp;

	for (cp = ustring, dp = lstring; *dp != '\0';)
	{
		*cp++ = islower (*dp) ? toupper (*dp) : *dp;
		++dp;
	}
	*cp = '\0';
}

/* convert nm.ex to a Unix style string */
char   *getname (nm, ex)
char   *nm, *ex;
{
    static char namebuf[14];
    register char  *cp, *dp;

    for (cp = namebuf, dp = nm; *dp != ' ' && dp != &nm[8];) {
	*cp++ = islower (*dp) ? toupper (*dp) : *dp;
	++dp;
    }
    *cp++ = '.';

    for (dp = ex; *dp != ' ' && dp != &ex[3];) {
	*cp++ = islower (*dp) ? toupper (*dp) : *dp;
	++dp;
    }

    *cp = '\0';
    return namebuf;
}

putname (cpmname, unixname)
char   *cpmname, *unixname;
{
    register char  *p1, *p2;

    for (p1 = unixname, p2 = cpmname; *p1; p1++, p2++) {
	while (*p1 == '.') {
	    p2 = cpmname + 8;
	    p1++;
	}
	if (p2 - cpmname < 11)
	    *p2 = islower(*p1) ? toupper(*p1) : *p1;
	else {
	    fprintf (stderr, "%s: name truncated\n", unixname);
	    break;
	}
    }
    while (p2 - cpmname < 11)
	*p2++ = ' ';
}

/* filarg - check if name matches argument list */
filarg (name)
char   *name;
{
    register int    i;

    if (nfiles <= 0)
	return 1;

    for (i = 0; i < nfiles; i++)
	if (match (name, fname[i])) {
	    ftouched[i] = true;
	    return 1;
	}

    return 0;
}

/* nopath - skip a d: drive prefix */
char   *nopath (name)
char   *name;
{
    return name[0] && name[1] == ':' ? name + 2 : name;
}

/* tofcb - name to the 11 character FCB form, * becomes ? */
tofcb (fcb, name)
char   *fcb, *name;
{
    register int    i;

    name = nopath (name);
    for (i = 0; i < 11; i++)
	fcb[i] = ' ';
    for (i = 0; i < 8 && *name && *name != '.'; name++)
	if (*name == '*')
	    while (i < 8)
		fcb[i++] = '?';
	else
	    fcb[i++] = islower (*name) ? toupper (*name) : *name;
    while (*name && *name != '.')
	name++;
    if (*name == '.')
	name++;
    for (i = 8; i < 11 && *name; name++)
	if (*name == '*')
	    while (i < 11)
		fcb[i++] = '?';
	else
	    fcb[i++] = islower (*name) ? toupper (*name) : *name;
}

/* match - does name match pattern, with ? wildcards */
match (name, pat)
char   *name, *pat;
{
    char    nf[11], pf[11];
    register int    i;

    tofcb (nf, name);
    tofcb (pf, pat);
    for (i = 0; i < 11; i++)
	if (pf[i] != '?' && pf[i] != nf[i])
	    return 0;
    return 1;
}

/* wild - does name have wildcards */
wild (name)
register char   *name;
{
    for (; *name; name++)
	if (*name == '*' || *name == '?')
	    return 1;
    return 0;
}

/*
 * expand - names of the disk files matching pat, with its drive prefix,
 * via BDOS search first/next. Returns the number found, at most max.
 */
expand (pat, names, max)
char   *pat;
char    names[][16];
int     max;
{
    char    fcb[36], dma[128];
    register char  *dp, *cp;
    register int    i, n, rc;

    for (i = 0; i < 36; i++)
	fcb[i] = 0;
    if (pat[0] && pat[1] == ':')
	fcb[0] = (islower (pat[0]) ? toupper (pat[0]) : pat[0]) - 'A' + 1;
    tofcb (fcb + 1, pat);

    bdos (26, dma);			/* set DMA */
    for (n = 0, rc = bdos (17, fcb); rc != 255 && n < max; rc = bdos (18, fcb)) {
	dp = dma + (rc & 3) * 32;
	cp = names[n++];
	if (pat != nopath (pat)) {
	    *cp++ = pat[0];
	    *cp++ = ':';
	}
	for (i = 1; i < 9 && (dp[i] & 0x7f) != ' '; i++)
	    *cp++ = dp[i] & 0x7f;
	*cp++ = '.';
	for (i = 9; i < 12 && (dp[i] & 0x7f) != ' '; i++)
	    *cp++ = dp[i] & 0x7f;
	*cp = '\0';
    }
    return n;
}

not_found () {
    register int    i;

    for (i = 0; i < nfiles; i++)
	if (!ftouched[i]) {
	    fprintf (stderr, "%s: not in library.\n", fname[i]);
	    errcnt++;
	}
}

extract(name)
char *name;
{
	getfiles(name, false);
}

print(name)
char *name;
{
	getfiles(name, true);
}

getfiles (name, pflag)
char   *name;
bool	pflag;
{
    FILE *lfd, *ofd;
    register int    i;
    char   *unixname;

    if ((lfd = fopenb (name, "r"))  == NULL)
	cant (name);

    ofd = pflag ? stdout : NULL;
    getdir (lfd);

    for (i = 1; i < nslots; i++) {
	if(ldir[i].l_stat != ACTIVE)
		continue;
	unixname = getname (ldir[i].l_name, ldir[i].l_ext);
	if (!filarg (unixname))
	    continue;
	fprintf(stderr,"%s", unixname);
	if (ofd != stdout)
	    ofd = fopenb (unixname, "w");
	if (ofd == NULL) {
	    fprintf (stderr, "  - can't create");
	    errcnt++;
	}
	else {
	    VOID fseek (lfd, (long) wtoi (ldir[i].l_off) * SECTOR, 0);
	    acopy (lfd, ofd, wtoi (ldir[i].l_len));
	    if (ofd != stdout)
		VOID fclose (ofd);
	}
	putc('\n', stderr);
    }
    VOID fclose (lfd);
    not_found ();
}

acopy (fdi, fdo, nsecs)
FILE *fdi, *fdo;
register unsigned int nsecs;
{
    register int    i, c;
#ifdef AZTEC
    int	    textfile = 0;	/* CP/M files are whole sectors: keep the ^Z */
#else
    int	    textfile = 1;
#endif

    while( nsecs-- != 0) 
	for(i=0; i<SECTOR; i++) {
		c = getc(fdi);
		if( feof(fdi) ) 
			error("Premature EOF\n");
		if( ferror(fdi) )
		    error ("Can't read");
		if( !isascii(c) )
		    textfile = 0;
		if( nsecs != 0 || !textfile || c != CTRLZ) {
			putc(c, fdo);
			if ( ferror(fdo) )
			    error ("write error");
		}
	 }
}

update (name)
char   *name;
{
    FILE *lfd;
    register int    i, j;
    int     n;
    static char names[MAXFILES][16];

    if ((lfd = fopenb (name, "r+")) == NULL) {
	if ((lfd = fopenb (name, "w+")) == NULL)
	    cant (name);
	initdir (lfd);
    }
    else
	getdir (lfd);		/* read old directory */
    endsec = libend ();

    if(verbose)
	    fprintf (stderr,"Updating files:\n");
    for (i = 0; i < nfiles; i++)
	if (wild (fname[i])) {
	    n = expand (fname[i], names, MAXFILES);
	    if (n == 0) {
		fprintf (stderr, "%s: no match\n", fname[i]);
		errcnt++;
	    }
	    for (j = 0; j < n; j++)
		addfil (names[j], lfd);
	}
	else
	    addfil (fname[i], lfd);
    if (errcnt == 0)
	putdir (lfd);
    else
	fprintf (stderr, "fatal errors - library not changed\n");
    VOID fclose (lfd);
}

addfil (name, lfd)
char   *name;
FILE *lfd;
{
    FILE	*ifd;
    register int secoffs, numsecs;
    register int i;

    if ((ifd = fopenb (name, "r")) == NULL) {
	fprintf (stderr, "%s: can't find to add\n",name);
	errcnt++;
	return;
    }
    for (i = 1; i < nslots; i++) {
	if (ldir[i].l_stat == ACTIVE &&
	    match (getname (ldir[i].l_name, ldir[i].l_ext), name)) {
	    fprintf (stderr, "Replacing %s\n", nopath (name));
	    break;
	}
	if (ldir[i].l_stat != ACTIVE) {
	    fprintf (stderr, "Adding %s\n", nopath (name));
	    break;
	}
    }
    if (i >= nslots) {
	fprintf (stderr, "%s: can't add library is full\n",name);
	errcnt++;
	VOID fclose (ifd);
	return;
    }

    ldir[i].l_stat = ACTIVE;
    putname (ldir[i].l_name, nopath (name));
    secoffs = endsec;		/* append to end */
    VOID fseek(lfd, (long) secoffs * SECTOR, 0);

    itow (ldir[i].l_off, secoffs);
    numsecs = fcopy (ifd, lfd);
    itow (ldir[i].l_len, numsecs);
    endsec += numsecs;
    VOID fclose (ifd);
}

fcopy (ifd, ofd)
FILE *ifd, *ofd;
{
    register int total = 0;
    register int i, n;
    char sectorbuf[SECTOR];


    while ( (n = fread( sectorbuf, 1, SECTOR, ifd)) != 0) {
	if (n != SECTOR)
	    for (i = n; i < SECTOR; i++)
		sectorbuf[i] = CTRLZ;
	if (fwrite( sectorbuf, 1, SECTOR, ofd ) != SECTOR)
		error("write error");
	++total;
    }
    return total;
}

delete (lname)
char   *lname;
{
    FILE *f;
    register int    i;

    if ((f = fopenb (lname, "r+")) == NULL)
	cant (lname);

    if (nfiles <= 0)
	error("delete by name only");

    getdir (f);
    for (i = 0; i < nslots; i++) {
	if (!filarg ( getname (ldir[i].l_name, ldir[i].l_ext)))
	    continue;
	ldir[i].l_stat = DELETED;
    }

    not_found();
    if (errcnt > 0)
	fprintf (stderr, "errors - library not updated\n");
    else
	putdir (f);
    VOID fclose (f);
}

reorg (name)
char  *name;
{
    FILE *olib, *nlib;
    int oldsize;
    register int i, j;
    static struct ludir odir[MAXFILES];
    char tmpname[SECTOR];
    register char *cp, *dp;

    /* same drive and name as the library, with a .$$$ type */
    for (cp = tmpname, dp = name; *dp && *dp != '.';)
	*cp++ = *dp++;
    VOID strcpy(cp, ".$$$");

    if( (olib = fopenb(name,"r")) == NULL)
	cant(name);

    if( (nlib = fopenb(tmpname, "w")) == NULL)
	cant(tmpname);

    getdir(olib);
    printf("Old library has %d slots\n", oldsize = nslots);
    for(i = 0; i < nslots ; i++)
	    copymem( (char *) &odir[i], (char *) &ldir[i],
			sizeof(struct ludir));
    initdir(nlib);
    endsec = libend();
    errcnt = 0;

    for (i = j = 1; i < oldsize; i++)
	if( odir[i].l_stat == ACTIVE ) {
	    if (j >= nslots) {
		errcnt++;
		fprintf(stderr, "Not enough room in new library\n");
		break;
	    }
	    if(verbose)
		fprintf(stderr, "Copying: %s\n",
			getname(odir[i].l_name, odir[i].l_ext));
	    copyentry( &odir[i], olib,  &ldir[j++], nlib);
        }

    VOID fclose(olib);
    putdir(nlib);
    VOID fclose (nlib);

    if(errcnt == 0) {
	if ( unlink(name) < 0 || rename(tmpname, name) < 0) {
	    VOID unlink(tmpname);
	    cant(name);
        }
    }
    else
	fprintf(stderr,"Errors, library not updated\n");
    VOID unlink(tmpname);

}

copyentry( old, of, new, nf )
struct ludir *old, *new;
FILE *of, *nf;
{
    register int secoffs, numsecs;
    char buf[SECTOR];

    new->l_stat = ACTIVE;
    copymem(new->l_name, old->l_name, 8);
    copymem(new->l_ext, old->l_ext, 3);
    VOID fseek(of, (long) wtoi(old->l_off)*SECTOR, 0);
    secoffs = endsec;
    VOID fseek(nf, (long) secoffs * SECTOR, 0);

    itow (new->l_off, secoffs);
    numsecs = wtoi(old->l_len);
    itow (new->l_len, numsecs);
    endsec += numsecs;

    while(numsecs-- != 0) {
	if( fread( buf, 1, SECTOR, of) != SECTOR)
	    error("read error");
	if( fwrite( buf, 1, SECTOR, nf) != SECTOR)
	    error("write error");
    }
}

copymem(dst, src, n)
register char *dst, *src;
register unsigned int n;
{
	while(n-- != 0)
		*dst++ = *src++;
}


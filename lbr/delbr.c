/*
 * A program to extract all files from a "Novosielski" archive (foo.LBR)
 *
 *	Jeff Martin, 10/9/82
 *
 *	07/28/84 - Fixed AZTEC conditional defines.  (Tried it this
 *		   time, instead of guessing). Removed DESMET stuff,
 *		   as the Read() function dosen't always know how many
 *		   bytes it has read!  That didn't work too well. <pjh>
 *
 *	06/10/84 - Kludged up to work with a variety of unix-like compilers.
 *		   BUFFERING added. <pjh>
 *
 *	04/07/84 - reworked to compile under DeSmet 'C' Compiler
 *		   for use under CP/M-86   Peter A. Polansky
 *
 *	01/01/83 - Bug in large LBR files removed. --JEC
 *
 *	11/12/83 - CP/M 68k version created.  Jim Cathey
 *
 *	01/03/83 - BDS version reworked to provide the generally-useful
 *		   getldir() function --JCM
 *
 *	01/02/83 - BDS C version created --JCM
 *
 *	11/27/82 - Fixed bug that was causing one-sector files to
 *		   be rejected as invalid directory entries  --JCM
 */


/* "#define" your compiler here to pick from following #define blocks */
/* DRC, C86, and AZTEC have all been tested */

#define AZTEC

#ifdef	DRC		/* digital research */
#define	VERSION		"1.1 6-Jun-84 DRC"
#include		<ctype.h>
#define	OPN		openb	/* open a file, binary mode		*/
#define FOPN		fopenb	/* open a buffered file, binary mode	*/
#define CRET		creatb	/* create a file, binary mode		*/
#define FO_BREAD	"r"	/* read mode for fopen()		*/
#define O_RDONLY	0	/* binary read for open()		*/
#define C_BWRITE	0	/* binary write for creat()		*/
				/* doesn't matter, DRC ignores mode on creat */
#define FGETC		fgetc
#endif

#ifdef CIC86		/* computer innovations */
#define	VERSION		"1.1 6-Jun-84 CI C86"
#define	OPN		open
#define FOPN		fopen
#define CRET		creat
#define FO_BREAD	"rb"
#define O_RDONLY	4
#define C_BWRITE	5
#define FGETC		fgetc
#endif

#ifdef AZTEC		/* aztec c ii v 1.05 */
#define	VERSION		"1.1 28-Jul-84 Aztec"
#define	OPN		open
#define FOPN		fopen
#define CRET		creat
#define FO_BREAD	"r"
#define C_BWRITE	0666
#define FGETC		getc
char*malloc();
#endif

#define NAMESIZE 	13	/* Max chars in filename.typ, plus null */
#define MAXDIRENT	64
#define TOOBIG		1024	/* Assume dir is bad if > this many sectors */
#define SECSIZ		128
#define BUFFSIZ		16384	/* be sure this is a multiple of 128 */
#define ERR		(-1)

#ifdef AZTEC
#include <stdio.h>
#include <fcntl.h>
#else
#include <stdio.h>
#endif

main(argc, argv)
int	argc;
char	*argv[];
{
	int	c, i;
	char	*s, *buf;
	char	filename[20];
	int	fdi, fdo;		/* file descriptors */
	int	ndir, dirent;
	char	names[MAXDIRENT][NAMESIZE];
	long	offsets[MAXDIRENT];
	unsigned sizes[MAXDIRENT];
	int	temp, toRead, didRead;
	long	lseek(), bytes;

	while (--argc > 0 && (*++argv)[0] == '-') {
		for (s = argv[0]+1; *s != '\0'; s++) {
			switch (*s) {
				default:
					printf("Illegal Option: '%c'\n", *s);
					argc = 0;
					break;
			}
		}
	}
	if (argc != 1) {
		printf("delbr v%s\n",VERSION);
		printf("Usage: delbr filename(.LBR assumed)\n");
		printf("Strip all files from a \"Novosielski\" archive.\n");
		exit(1);
	}
	
	if (!(buf=malloc(BUFFSIZ)))	/* get memory for buffer */
	{
		printf("Not enough memory.  ALLOC returned NULL\n");
		exit(0);
	}
	strcpy(filename,argv[0]);
	strcat(filename,".lbr");
	upcase(filename);

	if ((ndir= getldir(filename,MAXDIRENT,names,offsets,sizes)) == ERR) {
		printf("Trouble getting directory from %s\n", filename);
		exit (2);
	}
	
	fdi = OPN(filename, O_RDONLY);
	if (fdi == ERR) {
		printf("Cannot open %s.\n", filename);
		exit(2);
	}
	
	for (dirent = 0; dirent < ndir; dirent++) {
		
		if ((fdo = CRET(names[dirent], C_BWRITE)) == ERR) {
			printf("Cannot create %s.\n", names[dirent]);
			exit(2);
		}
		printf("Extracting: %-12s\n", names[dirent]);
		if (lseek(fdi, offsets[dirent], 0) == ERR) {
			printf("\nError seeking this entry - aborting.\n");
			exit(2);
		}

		bytes= (long) sizes[dirent] * 128L;
		while (bytes != 0L) {
			if (bytes > (long) BUFFSIZ) {
				toRead= BUFFSIZ;
				bytes-= BUFFSIZ;
			}
			else {
				toRead= bytes;
				bytes= 0L;
			}
			if ((didRead= read(fdi, buf, toRead)) != toRead) {
			  printf("\nError reading this entry - aborting.  Read %u bytes\n",didRead);
			  exit (2);
			}
			if (write(fdo, buf, toRead) != toRead) {
			  printf("\nError writing this entry - aborting.\n");
			  exit (2);
			}
		}
		close(fdo);
	}
	printf("\n");
	close(fdi);
}


/*
 * Get .LBR directory  -- names, offsets, and sizes of entries in an LBR file.
 *
 *   The returned function value is the number of actual entries found,
 *    or ERR (if couldn't open file).
 *
 *  Input parameters:
 *
 *   fname - pointer to the string containing the full pathname of the LBR
 *	file.
 *   maxent - maximum number of entries to get (i. e., usually the size of
 *	the following arrays).
 *   files - pointer to an array of filenames to be populated by this function.
 *	This array must have a column dimension of NAMESIZE.
 *   offsets - pointer to an array of file offsets, in units of bytes, to
 *	be populated by this function.
 *   sizes - pointer to an array of file sizes, in units of CPM sectors, to be
 *	populated by this function.
 */
getldir(fname, maxent, files, offsets, sizes)
char *fname;
int maxent;
char files[][NAMESIZE];
long offsets[];
unsigned sizes[];
{
	int	c, i, j;
	char	*s;
	char	entryname[20];
	FILE	*dir, *FOPN();
	int	ndir, dirent, nentry;
	long	ostart;
	unsigned osize;

	if ((dir = FOPN(fname, FO_BREAD)) == 0)
		return (ERR);

	fskip(dir, 14); /* Point to dir size */
	ndir = FGETC(dir);
	ndir = (ndir + (FGETC(dir)<<8))*4;
  
	fskip(dir, 16); /* Skip unused dir bytes */

	nentry = 0; /* Init count of live entries */
	for (dirent = 1; (dirent < ndir) && (nentry < maxent); dirent++) {
		
		if (FGETC(dir) != 0) {  /* Ignore defunct entries in dir */
			fskip(dir, 31);
			continue;
		}
		/* Build the next entryname */
		j = 0;
		for (i=1; i<=8; i++) {
			c = FGETC(dir) & 0x7f;
			c = tolower(c);
			if(c != ' ')
				entryname[j++] = c;
		}
		entryname[j++] = '.';
		for (i=1; i<=3; i++) {
			c = FGETC(dir) & 0x7f;
			c = tolower(c);
			if(c != ' ')
				entryname[j++] = c;
		}
		entryname[j] = '\0';

		upcase(entryname);

		/* Check for any no-no chars. */
		for (s = entryname; *s != '\0'; s++)
			if (*s == ';' || *s == '*' || *s < ' ')
				*s = '.';

		strcpy(files[nentry], entryname); /* Return the hacked name */

		/* Determine where this file is in the archive */
		ostart = FGETC(dir);
		ostart |= FGETC(dir) << 8;
		ostart *= SECSIZ;
		offsets[nentry] = ostart; /* Return the file's offset */
		
		osize = FGETC(dir);
		osize |= FGETC(dir) << 8;
		sizes[nentry] = osize; /* ... and its size */

		fskip(dir, 16); /* Skip unused dir bytes */
		nentry++;
	}
	fclose(dir);
	return (nentry);
}

/*
 * Skip specified number of bytes in the specified file
 */
fskip(file, bytes)
FILE *file;
int bytes;
{
	while (bytes-- > 0)
		FGETC(file);
}

/*
 * Uppercase a string
 */
upcase(str)
char *str;
{
	for ( ; *str; ++str)
		*str = toupper(*str);
}
TC(f

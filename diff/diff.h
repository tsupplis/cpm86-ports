/*
 * diff - common declarations (CP/M-86 port)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Minimal stat substitute for Aztec C86 / CP/M-86.
 * We only need st_mode to distinguish regular files; on CP/M-86
 * everything accessible is a regular file, so we stub it.
 */
struct stat {
	int	st_mode;
};
#define S_IFMT	 0170000
#define S_IFREG	 0100000
#define S_IFDIR	 0040000

/* stat() stub: always reports a regular file; returns -1 if cannot open */
#define stat(path, sb)	diff_stat(path, sb)
int	diff_stat(char *path, struct stat *sb);

/*
 * Output format options
 */
#define	D_NORMAL	0	/* Normal output */
#define	D_EDIT		-1	/* Editor script out */
#define	D_REVERSE	1	/* Reverse editor script */
#define	D_CONTEXT	2	/* Diff with context */
#define	D_IFDEF		3	/* Diff with merged #ifdef's */
#define	D_NREVERSE	4	/* Reverse ed script with numbered lines */

/* --- global variable declarations (defined in diff.c) --- */
extern	int	opt;
extern	int	tflag;
extern	int	bflag;
extern	int	wflag;
extern	int	iflag;
extern	int	wantelses;
extern	char	*ifdef1;
extern	char	*ifdef2;
extern	char	*endifname;
extern	int	inifdef;
extern	int	context;
extern	int	status;
extern	int	anychange;
extern	int	pflag;
extern	int	plines;
extern	int	oflag;
extern	char	*file1, *file2;
extern	struct	stat stb1, stb2;

char	*talloc();
char	*ralloc();
char	*savestr();
void	 done(int);
int	 min(int, int), max(int, int);
void	 diffreg();
void	 pageline();

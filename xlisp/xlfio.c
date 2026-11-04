/* xlfio - xlisp file i/o */

#include <stdio.h>

#include "xlisp.h"

/* external variables */
extern struct node *xlstack;

/* local variables */
static char buf[STRMAX+1];

/* xlfopen - open a file */
static struct node *xlfopen(args)
  struct node *args;
{
    struct node *oldstk,arg,fname,mode,*val;
    FILE *fp;

    /* create a new stack frame */
    oldstk = xlsave(&arg,&fname,&mode,0);

    /* initialize */
    arg.n_ptr = args;

    /* get the file name */
    fname.n_ptr = xlevmatch(STR,&arg.n_ptr);

    /* get the mode */
    mode.n_ptr = xlevmatch(STR,&arg.n_ptr);

    /* make sure there aren't any more arguments */
    xllastarg(arg.n_ptr);

    /* try to open the file */
    if ((fp = fopen(fname.n_ptr->n_str,
    			mode.n_ptr->n_str)) != 0) {
	val = newnode(FPTR);
	val->n_fp = fp;
    }
    else
	val = 0;

    /* restore the previous stack frame */
    xlstack = oldstk;

    /* return the file pointer */
    return (val);
}

/* xlfclose - close a file */
static struct node *xlfclose(args)
  struct node *args;
{
    struct node *fptr;

    /* get file pointer */
    fptr = xlevmatch(FPTR,&args);

    /* make sure there aren't any more arguments */
    xllastarg(args);

    /* make sure the file exists */
    if (fptr->n_fp == 0)
	xlfail("file not open");

    /* close the file */
    fclose(fptr->n_fp);
    fptr->n_fp = 0;

    /* return nil */
    return (0);
}

/* xlgetc - get a character from a file */
static struct node *xlgetc(args)
  struct node *args;
{
    struct node *val;
    FILE *fp;
    int ch;

    /* get file pointer */
    if (args != 0)
	fp = xlevmatch(FPTR,&args)->n_fp;
    else
	fp = stdin;

    /* make sure there aren't any more arguments */
    xllastarg(args);

    /* make sure the file exists */
    if (fp == 0)
	xlfail("file not open");

    /* get character and check for eof */
    if ((ch = getc(fp)) != EOF) {

	/* create return node */
	val = newnode(INT);
	val->n_int = ch;
    }
    else
	val = 0;

    /* return the character */
    return (val);
}

/* xlputc - put a character to a file */
static struct node *xlputc(args)
  struct node *args;
{
    struct node *oldstk,arg,chr;
    FILE *fp;

    /* create a new stack frame */
    oldstk = xlsave(&arg,&chr,0);

    /* initialize */
    arg.n_ptr = args;

    /* get the character */
    chr.n_ptr = xlevmatch(INT,&arg.n_ptr);

    /* get file pointer */
    if (arg.n_ptr != 0)
	fp = xlevmatch(FPTR,&arg.n_ptr)->n_fp;
    else
	fp = stdout;

    /* make sure there aren't any more arguments */
    xllastarg(arg.n_ptr);

    /* make sure the file exists */
    if (fp == 0)
	xlfail("file not open");

    /* put character to the file */
    putc(chr.n_ptr->n_int,fp);

    /* restore the previous stack frame */
    xlstack = oldstk;

    /* return the character */
    return (chr.n_ptr);
}

/* xlfgets - get a string from a file */
static struct node *xlfgets(args)
  struct node *args;
{
    struct node *oldstk,str,*val;
    char *sptr;
    FILE *fp;
    int len;

    oldstk = xlsave(&str,0);

    /* get file pointer */
    if (args != 0)
	fp = xlevmatch(FPTR,&args)->n_fp;
    else
	fp = stdin;

    /* make sure there aren't any more arguments */
    xllastarg(args);

    /* make sure the file exists */
    if (fp == 0)
	xlfail("file not open");

    /* get character and check for eof */
    if (fgets(buf,STRMAX,fp)) {

	/* create return node */
	str.n_ptr = newnode(STR);
	str.n_ptr->n_str = strsave(buf);

	/* make sure we got the whole string */
	while ((len = strlen(buf)) == 0 || buf[len-1] != '\n') {
	    if (!fgets(buf,STRMAX,fp))
		break;
	    sptr = str.n_ptr->n_str;
	    len = strlen(sptr);
	    if (len > 32766 - strlen(buf))
		xlfail("string too long");
	    str.n_ptr->n_str = stralloc(len + strlen(buf));
	    strcpy(str.n_ptr->n_str,sptr);
	    strcat(str.n_ptr->n_str,buf);
	    strfree(sptr);
	}
    }
    else
	str.n_ptr = 0;

    /* return the string */
    val = str.n_ptr;
    xlstack = oldstk;
    return (val);
}

/* xlfputs - put a string to a file */
static struct node *xlfputs(args)
  struct node *args;
{
    struct node *oldstk,arg,str;
    FILE *fp;

    /* create a new stack frame */
    oldstk = xlsave(&arg,&str,0);

    /* initialize */
    arg.n_ptr = args;

    /* get the string */
    str.n_ptr = xlevmatch(STR,&arg.n_ptr);

    /* get file pointer */
    if (arg.n_ptr != 0)
	fp = xlevmatch(FPTR,&arg.n_ptr)->n_fp;
    else
	fp = stdout;

    /* make sure there aren't any more arguments */
    xllastarg(arg.n_ptr);

    /* make sure the file exists */
    if (fp == 0)
	xlfail("file not open");

    /* put string to the file */
    fputs(str.n_ptr->n_str,fp);

    /* restore the previous stack frame */
    xlstack = oldstk;

    /* return the string */
    return (str.n_ptr);
}

/* xlfinit - initialize file stuff */
xlfinit()
{
    xlsubr("fopen",xlfopen);
    xlsubr("fclose",xlfclose);
    xlsubr("getc",xlgetc);
    xlsubr("putc",xlputc);
    xlsubr("fgets",xlfgets);
    xlsubr("fputs",xlfputs);
}

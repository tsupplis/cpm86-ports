int bp;			/*   next available char; init = 0 */
char buf[BUFSIZE];	/*   pushed-back chars */
char fcname[MAXNAME];	/*   text of current function name */
int fordep;		/*   current depth of for statements */
char forstk[MAXFORSTK];	/*   stack of reinit strings */
int swtop;		/*   current switch entry; init=0              */
int swlast;		/*   next available position; init=1           */
int swstak[MAXSWITCH];	/*   switch information stack                  */
int xfer;		/*   YES if just made transfer, NO otherwise */
int label;		/*   next label returned by labgen */
int level ;		/*   level of file inclusion; init = 1 */
int linect[NFILES];	/*   line count on input file[level]; init = 1 */
FILE *infile[NFILES];	/*   file number[level]; init infile[1] = STDIN */
int fnamp;		/*   next free slot in fnames; init = 2 */
char fnames[MAXFNAMES];	/*   stack of include names; init fnames[1] = EOS */
int avail;		/*   first first location in table; init = 1 */
int tabptr[127];	/*   name pointers; init = 0 */
int outp;		/*   last position filled in outbuf; init = 0 */
char outbuf[82];	/*   output lines collected here */
char fname[MAXNAME][NFILES];	/*   file names */
int nfiles;		/*   number of files */

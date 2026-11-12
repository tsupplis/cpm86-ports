/* Reorganization through the real utility, with injected commit failures. */
#include <stdio.h>
#include <stdlib.h>
static FILE *output;
static int mode, renames;
static FILE *test_fdopen(int fd, const char *flags)
{
    output=fdopen(fd,flags);
    return output;
}
static int test_close(FILE *f)
{
    int is_output = f==output;
    int result=fclose(f);
    if (mode==1 && is_output) return EOF;
    return result;
}
static int test_rename(const char *from, const char *to)
{
    if (++renames==2 && mode==2) return -1;
    return rename(from,to);
}
#define fdopen test_fdopen
#define fclose test_close
#define rename test_rename
#define main lu_main
#include "lu86.c"
#undef main
int main(int argc, char **argv)
{
    mode=atoi(argv[1]);
    lu_main(argc-1,argv+1);
    return 0;
}

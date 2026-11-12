/* File operations with allocation and I/O failures, without terminal input. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int allocations, fail_alloc, fail_close, fail_write, fail_rename;
static int fail_read, read_error;
static int test_getc(FILE *f)
{
    if (fail_read && --fail_read==0) { read_error=1; return EOF; }
    return getc(f);
}
static int test_ferror(FILE *f)
{
    return read_error || ferror(f);
}
static void *test_malloc(size_t n)
{
    if (++allocations==fail_alloc) return NULL;
    return malloc(n);
}
static void *test_realloc(void *p, size_t n)
{
    if (++allocations==fail_alloc) return NULL;
    return realloc(p,n);
}
static int test_close(FILE *f)
{
    int result = fclose(f);
    if (fail_close && --fail_close==0) return EOF;
    return result;
}
static size_t test_write(const void *p, size_t size, size_t n, FILE *f)
{
    if (fail_write) { fail_write=0; return 0; }
    return fwrite(p,size,n,f);
}
static int test_rename(const char *a, const char *b)
{
    if (fail_rename && --fail_rename==0) return -1;
    return rename(a,b);
}
#define malloc test_malloc
#define realloc test_realloc
#define fclose test_close
#define fwrite test_write
#define rename test_rename
#undef getc
#undef ferror
#define getc test_getc
#define ferror test_ferror
#define main kilo_main
#include "kilo.c"
#undef main
#undef malloc
#undef realloc
#undef fclose
#undef fwrite
#undef rename

static void need(int ok, char *message)
{
    if (!ok) { printf("FAIL: %s\n",message); exit(1); }
}

int main(int argc, char **argv)
{
    int mode, result;
    need(argc==2,"mode argument");
    mode=atoi(argv[1]);
    if (mode>=10 && mode<=18) fail_alloc=mode-9;
    if (mode==19) fail_alloc=3; /* growth of the line buffer */
    if (mode==20) fail_close=1; /* close of the input file */
    if (mode==21) fail_read=10; /* read failure in the second line */
    result=editorOpen("B:EDIT.TXT");
    if (mode>=10) {
        need(result<0,"allocation failure rejects partial load");
    } else {
        need(result==0 && E.numrows==2,"complete load with CP/M EOF");
        need(!strcmp(E.row[1].chars,"last"),"ignore stale tail after marker");
        memcpy(E.row[0].chars,"saved",5);
        E.dirty=1;
        fail_close=mode==1;
        fail_write=mode==2;
        fail_rename=mode==3 ? 2 : 0;
        allocations=0; fail_alloc=1;
        result=editorSave();
        need(allocations==0,"save allocates no document copy");
        need(result==(mode!=0),"save result");
        need((E.dirty!=0)==(mode!=0),"dirty retained on failed save");
    }
    puts("KILO PASS");
    return 0;
}

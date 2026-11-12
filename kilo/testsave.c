/* Exercise replacement failures using the target runtime's rename. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static int calls, failure;
static int faulty_rename(const char *from, const char *to)
{
    calls++;
    if (calls==failure || (failure==3 && calls==2)) return -1;
    return rename(from,to);
}
#define rename faulty_rename
#include "fsave.h"
#undef rename

static void need(int ok, char *message)
{
    if (!ok) { printf("FAIL: %s\n",message); exit(1); }
}

int main(int argc, char **argv)
{
    FILE *f;
    char temp[FS_NAME], buf[16];
    int result;

    need(argc==2,"mode argument");
    failure = atoi(argv[1]);
    f = fs_create(temp,sizeof(temp),"B:BASE.DAT");
    need(f!=NULL && !strncmp(temp,"B:",2),"same drive temporary");
    need(strcmp(temp,"B:FS0000.TMP"),"existing temporary preserved");
    need(fputs("NEW",f)!=EOF && fclose(f)==0,"finish temporary");
    result = fs_commit(temp,"B:BASE.DAT",failure==4 ? "B:BASE.BAK" :
                       (failure==5 ? "b:base.dat" : NULL));
    need(result==(failure==0 || failure>=4),"commit result");
    if (failure!=3) {
        f=fopen("B:BASE.DAT","r"); need(f!=NULL,"destination exists");
        memset(buf,0,sizeof(buf)); fread(buf,1,3,f); fclose(f);
        need(!strcmp(buf,result ? "NEW" : "OLD"),"destination contents");
    }
    puts("SAVE PASS");
    return 0;
}

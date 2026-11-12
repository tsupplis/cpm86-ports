/* Run against the real Aztec runtime; fail each write in a split in turn. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static int fail_at, writes, partial;
static int faulty_write(int fd, char *buf, int n)
{
    if (fail_at && ++writes == fail_at) {
        if (partial) return write(fd,buf,n/2);
        return -1;
    }
    return write(fd,buf,n);
}
#define write faulty_write
#include "ndbm.c"
#undef write

static void need(int ok, char *message)
{
    if (!ok) { printf("FAIL: %s\n",message); exit(1); }
}

int main(void)
{
    char keys[3], values[900];
    datum k[3], v, got;
    DBM *db;
    FILE *f;
    int i, candidate, replace, point, mode, count, seen;

    for (i=0,candidate=1; i<3; candidate++) {
        keys[i] = candidate;
        k[i].dptr = keys+i; k[i].dsize = 1;
        if ((dcalchash(k[i]) & 1) == (i == 1)) i++;
    }
    memset(values,'V',sizeof(values)); v.dptr = values;
    for (replace=0; replace<2; replace++)
    for (mode=0; mode<2; mode++)
    for (point=1; point<=4; point++) {
        fail_at = 0;
        f = fopen("FAULT.pag","w"); need(f!=NULL,"create page"); fclose(f);
        f = fopen("FAULT.dir","w"); need(f!=NULL,"create directory"); fclose(f);
        db = dbm_open("FAULT",O_RDWR,0); need(db!=NULL,"open");
        v.dsize = 400;
        need(dbm_store(db,k[0],v,DBM_INSERT)==0,"seed zero");
        need(dbm_store(db,k[1],v,DBM_INSERT)==0,"seed one");
        fail_at = point; writes = 0; partial = mode;
        v.dsize = replace ? 800 : 400;
        need(dbm_store(db,k[replace ? 0 : 2],v,DBM_REPLACE)<0,"write failure returned");
        need(writes>=point && dbm_error(db),"injection reached and error flagged");
        fail_at = 0;
        dbm_clearerr(db);
        for (i=0; i<2; i++) {
            got = dbm_fetch(db,k[i]);
            need(got.dptr && got.dsize==400 && !memcmp(got.dptr,values,400),"cached old value");
        }
        dbm_close(db);
        db = dbm_open("FAULT",O_RDONLY,0); need(db!=NULL,"reopen");
        for (i=0; i<2; i++) {
            got = dbm_fetch(db,k[i]);
            need(got.dptr && got.dsize==400 && !memcmp(got.dptr,values,400),"persisted old value");
        }
        got = dbm_fetch(db,k[2]); need(!got.dptr,"failed insert absent");
        need(!dbm_error(db),"read after failure");
        count=seen=0;
        for (got=dbm_firstkey(db); got.dptr; got=dbm_nextkey(db)) {
            need(got.dsize==1,"iteration key size");
            i=*got.dptr==keys[0] ? 1 : (*got.dptr==keys[1] ? 2 : 0);
            need(i && !(seen&i),"no orphan/duplicate keys after failure");
            seen|=i; count++;
        }
        need(count==2 && !dbm_error(db),"iteration after failure");
        dbm_close(db);
    }
    puts("FAULT PASS: 16 split/replacement short-write and error cases");
    return 0;
}

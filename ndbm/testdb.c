/* Small binary-persistence and write-error acceptance tests. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "ndbm.h"

static char keybuf[4], value[1024];
static int checks;

static void need(ok, message)
int ok;
char *message;
{
    if (!ok) { printf("FAIL: %s (check %d)\n", message,checks); exit(1); }
    checks++;
}

static datum key(i)
int i;
{
    datum d;
    keybuf[0] = 0; keybuf[1] = i; keybuf[2] = i >> 8; keybuf[3] = 255;
    d.dptr = keybuf; d.dsize = 4;
    return d;
}

static datum val(i, changed)
int i, changed;
{
    datum d;
    int j;
    d.dsize = changed ? 60 : 40 + i % 100;
    for (j = 0; j < d.dsize; j++) value[j] = (i * 17 + j * 31 + changed) & 255;
    d.dptr = value;
    return d;
}

static void verify(db, changed, deleted)
DBM *db;
int changed, deleted;
{
    int i;
    datum k, expected, got;
    for (i = 0; i < 12; i++) {
        k = key(i); got = dbm_fetch(db, k);
        if (deleted && i % 3 == 0) need(got.dptr == NULL, "deleted key persisted");
        else {
            expected = val(i, changed && i % 2 == 0);
            need(got.dptr && got.dsize == expected.dsize &&
                 memcmp(got.dptr, expected.dptr, got.dsize) == 0, "binary value round trip");
        }
    }
    need(!dbm_error(db), "database read error");
}

int main(argc, argv)
int argc;
char **argv;
{
    DBM *db;
    datum k, v, got;
    int i, n, seen[12];
    db = dbm_open("BINARY", O_RDWR, 0666);
    need(db != NULL, "create database");
    for (i = 0; i < 12; i++) {
        if (dbm_store(db,key(i),val(i,0),DBM_INSERT)!=0) { printf("FAIL: store binary pair %d flags %d\n",i,db->dbm_flags); exit(1); }
        checks++;
    }
    verify(db,0,0);
    need(dbm_store(db,key(0),val(1,1),DBM_INSERT)==1,"duplicate insert rejected");
    verify(db,0,0);
    dbm_close(db);
    db = dbm_open("BINARY", O_RDWR, 0); need(db != NULL,"reopen");
    verify(db,0,0);
    for (i=0; i<12; i+=2) need(dbm_store(db,key(i),val(i,1),DBM_REPLACE)==0,"replace growing value");
    for (i=0; i<12; i+=3) need(dbm_delete(db,key(i))==0,"delete");
    dbm_close(db);
    db=dbm_open("BINARY",O_RDONLY,0); need(db != NULL,"reopen read only");
    verify(db,1,1);
    need(dbm_store(db,key(1),val(0,0),DBM_REPLACE)<0,"read-only store rejected");
    need(dbm_delete(db,key(1))<0,"read-only delete rejected");
    memset(seen,0,sizeof(seen)); n=0;
    for (k=dbm_firstkey(db); k.dptr; k=dbm_nextkey(db)) {
        need(k.dsize==4,"iteration key size");
        i=(unsigned char)k.dptr[1]+256*(unsigned char)k.dptr[2];
        need(i<12 && i%3!=0 && !seen[i],"iteration returns each surviving key once");
        seen[i]=1; n++;
    }
    need(n==8 && !dbm_error(db),"iteration count");
    dbm_close(db);
    /* An oversized replacement must not destroy the previous on-disk value. */
    db=dbm_open("BINARY",O_RDWR,0); need(db!=NULL,"reopen oversized test");
    v.dptr=value; v.dsize=1024;
    need(dbm_store(db,key(1),v,DBM_REPLACE)<0,"oversized datum rejected");
    dbm_close(db);
    db=dbm_open("BINARY",O_RDWR,0); need(db!=NULL,"reopen failed replacement");
    verify(db,1,1);
    /* Force a genuine write failure while leaving the disk contents intact. */
    close(db->dbm_pagf);
    db->dbm_pagf=open("BINARY.pag",O_RDONLY,0);
    need(db->dbm_pagf>=0,"open write fault descriptor");
    need(dbm_store(db,key(1),val(9,1),DBM_REPLACE)<0 && dbm_error(db),"failed write reported");
    dbm_close(db);
    db=dbm_open("BINARY",O_RDONLY,0); need(db!=NULL,"reopen after write failure");
    verify(db,1,1); dbm_close(db);
    printf("INTEGRITY PASS %d\n",checks); exit(0);
}

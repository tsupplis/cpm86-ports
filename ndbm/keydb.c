/*
 * keydb -- simple string->string map tool backed by ndbm
 *
 * Usage:
 *   keydb add  <dbname> <key> <value>   store (or replace) a key
 *   keydb get  <dbname> <key>           fetch and print value
 *   keydb del  <dbname> <key>           delete a key
 *   keydb list <dbname>                 list all key=value pairs
 *
 * The database is stored as <dbname>.dir and <dbname>.pag.
 * Files are created automatically by "add" when they do not yet exist.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include "ndbm.h"

static void strlower(s)
    char *s;
{
    for (; *s; s++) *s = tolower(*s);
}

static void usage()
{
    fputs("usage: keydb add  <db> <key> <value>\n", stderr);
    fputs("       keydb get  <db> <key>\n", stderr);
    fputs("       keydb del  <db> <key>\n", stderr);
    fputs("       keydb list <db>\n", stderr);
    exit(1);
}

/* Make a datum from a NUL-terminated string (includes the NUL). */
static datum str_datum(s)
    char *s;
{
    datum d;
    d.dptr  = s;
    d.dsize = strlen(s) + 1;
    return d;
}

/* Open db read-write, creating the .dir/.pag files if absent. */
static DBM *open_rw(name)
    char *name;
{
    DBM *db;
    char path[128];
    int fd;

    /* Create .dir if missing */
    strcpy(path, name);
    strcat(path, ".dir");
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) { fprintf(stderr, "keydb: can't open %s\n", path); exit(1); }
    close(fd);

    /* Create .pag if missing */
    strcpy(path, name);
    strcat(path, ".pag");
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) { fprintf(stderr, "keydb: can't open %s\n", path); exit(1); }
    close(fd);

    db = dbm_open(name, O_RDWR, 0666);
    if (!db) { fprintf(stderr, "keydb: can't open db %s\n", name); exit(1); }
    return db;
}

static DBM *open_ro(name)
    char *name;
{
    DBM *db = dbm_open(name, O_RDONLY, 0);
    if (!db) { fprintf(stderr, "keydb: can't open db %s\n", name); exit(1); }
    return db;
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    char *cmd, *dbname;
    DBM  *db;
    datum key, val;

    if (argc < 3) usage();
    cmd    = argv[1];
    dbname = argv[2];

    strlower(cmd);  /* only cmd is lowered; dbname and args keep their case */

    if (strcmp(cmd, "add") == 0) {
        if (argc != 5) usage();
        db  = open_rw(dbname);
        key = str_datum(argv[3]);
        val = str_datum(argv[4]);
        if (dbm_store(db, key, val, DBM_REPLACE) < 0) {
            fputs("keydb: store failed\n", stderr);
            dbm_close(db);
            return 1;
        }
        dbm_close(db);

    } else if (strcmp(cmd, "get") == 0) {
        if (argc != 4) usage();
        db  = open_ro(dbname);
        key = str_datum(argv[3]);
        val = dbm_fetch(db, key);
        if (val.dptr == NULL) {
            fputs("keydb: key not found\n", stderr);
            dbm_close(db);
            return 1;
        }
        puts(val.dptr);
        dbm_close(db);

    } else if (strcmp(cmd, "del") == 0) {
        if (argc != 4) usage();
        db  = open_rw(dbname);
        key = str_datum(argv[3]);
        if (dbm_delete(db, key) < 0) {
            fputs("keydb: key not found or delete failed\n", stderr);
            dbm_close(db);
            return 1;
        }
        dbm_close(db);

    } else if (strcmp(cmd, "list") == 0) {
        if (argc != 3) usage();
        db  = open_ro(dbname);
        key = dbm_firstkey(db);
        while (key.dptr != NULL) {
            val = dbm_fetch(db, key);
            if (val.dptr != NULL)
                printf("%s=%s\n", key.dptr, val.dptr);
            key = dbm_nextkey(db);
        }
        dbm_close(db);

    } else {
        usage();
    }

    return 0;
}

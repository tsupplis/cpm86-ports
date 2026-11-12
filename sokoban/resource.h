#ifndef SOKOBAN_RESOURCE_H
#define SOKOBAN_RESOURCE_H
#include <stdio.h>

#define SK_LEVELS 0
#define SK_HELP 1

typedef struct {
    FILE *file;
    unsigned segment, position, end;
} sk_resource;

int sk_resource_open(sk_resource *resource, int id);
int sk_resource_getc(sk_resource *resource);
void sk_resource_close(sk_resource *resource);
#endif

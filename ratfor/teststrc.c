#include <stdio.h>
#define EOS -2

int putchar_(int *c)
{
    return putchar(*c);
}

void putstr_(int *c)
{
    int *p;
    for (p = c; *p != EOS; ++p) {
	putchar(*p);
    }
}

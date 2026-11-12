/* Same tests compiled once for CP/M memory and once for DOS file resources. */
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

static void need(int ok)
{
    if (!ok) { puts("FAIL: resource reader"); exit(1); }
}

int main(int argc, char **argv)
{
    sk_resource resources[2], invalid;
    unsigned hashes[2];
    long lengths[2];
    int i, c, active;

    need(!sk_resource_open(&invalid, -1));
    if (argc > 1) {
        need(!sk_resource_open(&invalid, SK_LEVELS));
        need(!sk_resource_open(&invalid, SK_HELP));
        puts("MISSING PASS");
        return 0;
    }
    for (i = 0; i < 2; ++i) {
        need(sk_resource_open(&resources[i], i));
        hashes[i] = 0; lengths[i] = 0;
    }
    do {
        active = 0;
        for (i = 0; i < 2; ++i) {
            c = sk_resource_getc(&resources[i]);
            if (c != EOF) {
                hashes[i] = hashes[i]*33U + c;
                ++lengths[i]; active = 1;
            }
        }
    } while (active);
    for (i = 0; i < 2; ++i) {
        need(sk_resource_getc(&resources[i]) == EOF);
        printf("RESOURCE %d %ld %u\n", i, lengths[i], hashes[i]);
        sk_resource_close(&resources[i]);
        need(sk_resource_getc(&resources[i]) == EOF);
        need(sk_resource_open(&resources[i], i));
        need(sk_resource_getc(&resources[i]) != EOF);
        sk_resource_close(&resources[i]);
    }
    puts("RESOURCE PASS");
    return 0;
}

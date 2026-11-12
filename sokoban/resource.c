#include "resource.h"
#if defined(__CPM86__)
#include <regs.h>
#include <x86.h>
#endif

int sk_resource_open(sk_resource *r, int id)
{
#if defined(__CPM86__)
    struct SREGS regs;
    unsigned segment, levels, help, help_offset;
    long capacity, end;
#endif
    r->file = NULL;
    r->segment = r->position = r->end = 0;
    if (id != SK_LEVELS && id != SK_HELP) return 0;
#if defined(__CPM86__)
    /* EXTRA descriptor in the Aztec DATA base page: length-minus-one,
     * then base paragraph. Do not rely on ES surviving C startup. */
    segread(&regs);
    segment = peekw(0x0f, regs.ds);
    capacity = (long)peekw(0x0c, regs.ds) +
               ((long)(peekb(0x0e, regs.ds) & 15) << 16) + 1;
    if (!segment || capacity < 12 || capacity > 65536L) return 0;
    if (peekb(0, segment) != 'S' || peekb(1, segment) != 'K' ||
        peekb(2, segment) != 'R' || peekb(3, segment) != '1') return 0;
    levels = peekw(6, segment);
    help_offset = peekw(8, segment);
    help = peekw(10, segment);
    end = (long)help_offset + help;
    if (peekw(4, segment) != 12 || !levels || !help ||
        (long)levels + 12 != help_offset || end > 65535L || end > capacity)
        return 0;
    r->segment = segment;
    r->position = id == SK_LEVELS ? 12 : help_offset;
    r->end = id == SK_LEVELS ? help_offset : (unsigned)end;
    return 1;
#else
    /* DOS and other file-based ports retain their external resources. */
    r->file = fopen(id == SK_LEVELS ? "soklevls.dat" : "sokoban.hlp", "r");
    return r->file != NULL;
#endif
}

int sk_resource_getc(sk_resource *r)
{
#if defined(__CPM86__)
    if (r->position == r->end) return EOF;
    return peekb(r->position++, r->segment) & 255;
#else
    int c;
    if (!r->file || r->end) return EOF;
    c = getc(r->file);
    if (c == EOF || c == 0 || c == 0x1a) {
        r->end = 1;
        return EOF;
    }
    return c;
#endif
}

void sk_resource_close(sk_resource *r)
{
    if (r->file) fclose(r->file);
    r->file = NULL;
    r->position = r->end = 0;
}

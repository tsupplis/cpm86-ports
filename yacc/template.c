#include <stdio.h>
#include <regs.h>
#include <x86.h>
#include "template.h"

#if !__CPM86__
#error The embedded parser template requires CP/M-86
#endif

static unsigned segment, position, end;

int template_init(void)
{
    struct SREGS regs;
    unsigned length;
    long capacity;

    /* Aztec keeps DS at the DATA base page. The EXTRA descriptor at 0Ch
     * holds a 20-bit length-minus-one and a base paragraph at 0Fh.
     * Read the descriptor rather than assuming ES survives C startup. */
    segread(&regs);
    segment = peekw(0x0f, regs.ds);
    position = end = 0;
    capacity = (long)peekw(0x0c, regs.ds) +
               ((long)(peekb(0x0e, regs.ds) & 15) << 16) + 1;
    if (!segment || capacity < 6 || capacity > 65536L)
        return 0;
    if (peekb(0, segment) != 'Y' || peekb(1, segment) != 'T' ||
        peekb(2, segment) != 'P' || peekb(3, segment) != 'L')
        return 0;
    length = peekw(4, segment);
    if (!length || length > 65529U || (long)length + 6 > capacity)
        return 0;
    position = 6;
    end = length + 6;
    return 1;
}

int template_getc(void)
{
    if (position == end) return EOF;
    return peekb(position++, segment) & 255;
}

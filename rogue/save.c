/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */
#include "rogue.h"

char *save_file = (char *) 0;
short write_failed = 0;		/* used by save overlay, reset on each save */

void
save_game()
{
	ovloader("save", 1, (void *) 0);
}

void
save_into_file(sfile)
        char *sfile;
{
	ovloader("save", 2, (void *) sfile);
}

void
restore(fname)
        char *fname;
{
	ovloader("save", 3, (void *) fname);
}

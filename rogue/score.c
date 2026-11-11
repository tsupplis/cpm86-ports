/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */
#include "rogue.h"

void
killed_by(monster, other)
        object *monster;
        int other;
{
	ovloader("score", 1, (int) monster, other);
}

void
win()
{
	ovloader("score", 2, 0, 0);
}

void
quit(from_intrpt)
        boolean from_intrpt;
{
	ovloader("score", 3, (int) from_intrpt, 0);
}

void
put_scores(monster, other)
        object *monster;
        int other;
{
	ovloader("score", 4, (int) monster, other);
}

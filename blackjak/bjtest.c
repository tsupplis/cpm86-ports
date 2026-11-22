#include <assert.h>
#include <string.h>
#include "rules.h"

static int first(int n)
{
    assert(n >= 2 && n <= DECK);
    return 0;
}

int main(void)
{
    struct hand player, dealer;
    unsigned char deck[DECK], seen[DECK];
    int i;

    memset(&player, 0, sizeof(player));
    addcard(&player, 0);
    addcard(&player, 13);
    assert(total(&player) == 12 && allowed(&player, SPLIT, 1));
    assert(!allowed(&player, SPLIT, 0));
    addcard(&player, 9);
    assert(total(&player) == 12 && !allowed(&player, DOUBLE, 0));
    addcard(&player, 8);
    assert(total(&player) == 21 && !natural(&player));

    memset(&dealer, 0, sizeof(dealer));
    addcard(&dealer, 0);
    addcard(&dealer, 5);
    assert(total(&dealer) == 17 && dealerhit(&dealer));
    addcard(&dealer, 9);
    assert(total(&dealer) == 17 && !dealerhit(&dealer));

    memset(&player, 0, sizeof(player));
    addcard(&player, 9);
    addcard(&player, 10);
    assert(allowed(&player, DOUBLE, 1) && !allowed(&player, SPLIT, 1));
    player.bet = 10;
    assert(settle(&player, &dealer) == 20);
    addcard(&player, 0);
    assert(natural(&player) == 0);
    dealer = player;
    assert(settle(&player, &dealer) == 0);
    addcard(&player, 1);
    addcard(&dealer, 2);
    assert(settle(&player, &dealer) == -20);
    player.count = 2;
    player.hard = 11;
    assert(natural(&player));
    assert(insure(5, 1) == 10 && insure(5, 0) == -5);

    memset(seen, 0, sizeof(seen));
    shuffle(deck, first);
    for (i = 0; i < DECK; ++i) {
        assert(deck[i] < DECK && !seen[deck[i]]);
        seen[deck[i]] = 1;
    }
    return 0;
}

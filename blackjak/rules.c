#include "rules.h"

void addcard(struct hand *h, int card)
{
    int rank = card % 13;
    if (h->count == 0)
        h->first = card;
    if (h->count == 1)
        h->second = card;
    ++h->count;
    h->hard += rank < 9 ? rank + 1 : 10;
    if (rank == 0)
        ++h->aces;
}

static int soft(struct hand *h)
{
    return h->aces && h->hard <= 11;
}

int total(struct hand *h)
{
    return h->hard + (soft(h) ? 10 : 0);
}

int natural(struct hand *h)
{
    return h->count == 2 && total(h) == 21;
}

int dealerhit(struct hand *h)
{
    int value = total(h);
    return value < 17 || (value == 17 && soft(h));
}

int allowed(struct hand *h, int action, int cansplit)
{
    switch (action) {
    case 0: return 1;
    case HIT: return total(h) < 21;
    case DOUBLE: return h->count == 2 && total(h) < 21;
    case SPLIT:
        return cansplit && h->count == 2 &&
               h->first % 13 == h->second % 13;
    }
    return 0;
}

/* Net profit, in half dollars. Naturals are settled before normal play. */
long settle(struct hand *player, struct hand *dealer)
{
    int p = total(player), d = total(dealer);
    if (p > 21 || (d <= 21 && d > p))
        return -2L * player->bet;
    if (d > 21 || p > d)
        return 2L * player->bet;
    return 0;
}

long insure(int bet, int blackjack)
{
    return blackjack ? 2L * bet : -(long)bet;
}

void shuffle(unsigned char *deck, int (*pick)(int))
{
    int i, j, card;
    for (i = 0; i < DECK; ++i)
        deck[i] = i;
    for (i = DECK - 1; i > 0; --i) {
        j = (*pick)(i + 1);
        card = deck[j];
        deck[j] = deck[i];
        deck[i] = card;
    }
}

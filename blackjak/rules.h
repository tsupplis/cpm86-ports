#ifndef BJ_RULES_H
#define BJ_RULES_H

#define DECK 52
#define HIT 1
#define DOUBLE 2
#define SPLIT 3

/* Cards are numbered 0..51, with rank 0 (ace) through 12 (king). */
struct hand {
    int hard, aces, count, first, second, bet;
};

void addcard(struct hand *, int);
int total(struct hand *);
int natural(struct hand *);
int dealerhit(struct hand *);
int allowed(struct hand *, int, int);
long settle(struct hand *, struct hand *);
long insure(int, int);
void shuffle(unsigned char *, int (*)(int));

#endif

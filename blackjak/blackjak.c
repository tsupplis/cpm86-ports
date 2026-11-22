#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rules.h"

/* Three hands can use at most 93 hard points, even counting busts.
 * The lowest 26 cards in a deck already sum to 98. */
#define ROUND_CARDS 26

static unsigned char deck[DECK];
static int next = DECK;
static int suit_glyphs;

/* -1 means end of input, -2 means an invalid or overlong line. */
static int number(char *prompt)
{
    int c, value = 0, digits = 0, trailing = 0, bad = 0;
    fputs(prompt, stdout);
    fflush(stdout);
    while ((c = getchar()) != EOF && c != '\n' && c != 26) {
        if (c == ' ' || c == '\t' || c == '\r') {
            trailing = digits;
        } else if (c >= '0' && c <= '9' && !trailing && value < 1000) {
            value = value * 10 + c - '0';
            digits = 1;
        } else {
            bad = 1;
        }
    }
    if ((c == EOF || c == 26) && !digits && !bad)
        return -1;
    return digits && !bad ? value : -2;
}

static int pick(int n)
{
    return rand() % n;
}

static void reshuffle(void)
{
    shuffle(deck, pick);
    next = 0;
    puts("Deck shuffled.");
}

/* CP437 frames and optional PC text-console suit glyphs. */
#define FACEDOWN (-1)
#define ONE_CARD (-2)

#ifdef CPM_VT52
static void cardcolor(int color)
{
    /* CP/M-86 VT52 foreground extension takes a raw PC palette index. */
    static char sequence[] = {27, 'j', 27, 'b', 0, 27, 'k'};
    sequence[4] = color;
    fwrite(sequence, 1, sizeof(sequence), stdout);
}
#endif

static void cardrow(int card, int row)
{
    static char *rank[] = {
        "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
    };
    static char glyph[] = {6, 4, 3, 5}; /* spades, diamonds, hearts, clubs */
    int suit;
    static char *edge[] = {"\332\304\304\304\304\277", "\300\304\304\304\304\331"};

#ifdef CPM_VT52
    if (card >= 0)
        cardcolor(card / 13 == 1 || card / 13 == 2 ? 12 : 9);
#endif
    if (row != 1)
        fputs(edge[row / 2], stdout);
    else if (card == FACEDOWN)
        fputs("\263####\263", stdout);
    else {
        suit = card / 13;
        printf("\263%2s%c \263", rank[card % 13],
               suit_glyphs ? glyph[suit] : "SDHC"[suit]);
    }
#ifdef CPM_VT52
    if (card >= 0)
        cardcolor(7);
#endif
}

static void cards(int first, int second)
{
    int row;
    for (row = 0; row < 3; ++row) {
        cardrow(first, row);
        if (second != ONE_CARD) {
            putchar(' ');
            cardrow(second, row);
        }
        putchar('\n');
    }
}

static int deal(struct hand *h)
{
    int card = deck[next++];
    addcard(h, card);
    return card;
}

static void draw(struct hand *h, char *label)
{
    int card = deal(h);
    printf("%s (%d)\n", label, total(h));
    cards(card, ONE_CARD);
}

static void reveal(struct hand *dealer)
{
    printf("Dealer reveals (%d)\n", total(dealer));
    cards(dealer->first, dealer->second);
}

static int action(struct hand *h, int cansplit)
{
    int choice;
    do {
        choice = number("0 stand, 1 hit, 2 double, 3 split: ");
        if (choice == -1 || allowed(h, choice, cansplit))
            return choice;
        puts("That action is unavailable.");
    } while (1);
}

static int play(struct hand *h, int choice)
{
    while (choice > 0) {
        draw(h, "You draw");
        if (choice == DOUBLE) {
            h->bet *= 2;
            break;
        }
        if (total(h) >= 21)
            break;
        choice = action(h, 0);
    }
    return choice != -1;
}

static int playround(int bet, long *profit)
{
    struct hand dealer, player[2];
    int choice, count = 1, i, splitace, live = 0;
    long net = 0, result;

    memset(&dealer, 0, sizeof(dealer));
    memset(player, 0, sizeof(player));
    player[0].bet = bet;
    deal(&dealer);
    deal(&dealer);
    deal(&player[0]);
    deal(&player[0]);
    puts("Dealer shows");
    cards(dealer.first, FACEDOWN);
    printf("Your hand (%d)\n", total(&player[0]));
    cards(player[0].first, player[0].second);

    if (dealer.first % 13 == 0) {
        do {
            choice = number("Insurance (1 yes, 0 no): ");
        } while (choice != -1 && choice != 0 && choice != 1);
        if (choice == -1)
            return 0;
        if (choice) {
            net = insure(bet, natural(&dealer));
            puts(net > 0 ? "Insurance wins." : "Insurance loses.");
        }
    }
    if (natural(&player[0]) || natural(&dealer)) {
        reveal(&dealer);
        if (natural(&player[0]) && natural(&dealer))
            puts("Both have blackjack. Push.");
        else if (natural(&player[0])) {
            puts("Blackjack. You win 3:2.");
            net += 3L * bet;
        } else {
            puts("Dealer blackjack.");
            net -= 2L * bet;
        }
    } else {
        choice = action(&player[0], 1);
        if (choice == SPLIT) {
            addcard(&player[1], player[0].second);
            player[1].bet = bet;
            i = player[0].first;
            memset(&player[0], 0, sizeof(player[0]));
            player[0].bet = bet;
            addcard(&player[0], i);
            splitace = i % 13 == 0;
            count = 2;
            for (i = 0; i < count; ++i) {
                deal(&player[i]);
                printf("Hand %d (%d)\n", i + 1, total(&player[i]));
                cards(player[i].first, player[i].second);
                if (!splitace && total(&player[i]) < 21 &&
                    !play(&player[i], action(&player[i], 0)))
                    return 0;
            }
        } else if (!play(&player[0], choice))
            return 0;

        reveal(&dealer);
        for (i = 0; i < count; ++i)
            if (total(&player[i]) <= 21)
                live = 1;
        while (live && dealerhit(&dealer))
            draw(&dealer, "Dealer draws");

        for (i = 0; i < count; ++i) {
            result = settle(&player[i], &dealer);
            printf("Hand %d (%d): %s.\n", i + 1, total(&player[i]),
                   result > 0 ? "win" : result < 0 ? "lose" : "push");
            net += result;
        }
    }
    *profit += net;
    return 1;
}

int main(int argc, char **argv)
{
    int bet;
    long profit = 0;
    unsigned long amount;
    if (argc > 1) {
        if (argc != 2 || (strcmp(argv[1], "-g") != 0 &&
                          strcmp(argv[1], "-G") != 0)) {
            puts("Usage: BLACKJAK [-g]");
            return 1;
        }
        suit_glyphs = 1;
    }
    srand((unsigned)time((time_t *)0));
    puts("Blackjack | Dealer hits soft 17 | Blackjack pays 3:2");
    for (;;) {
        bet = number("Wager ($1-500, 7777 shuffle, 0 quit): ");
        if (bet == -1 || bet == 0)
            break;
        if (bet == 7777)
            reshuffle();
        else if (bet < 1 || bet > 500)
            puts("Enter a whole-dollar wager from 1 to 500.");
        else {
            if (DECK - next < ROUND_CARDS)
                reshuffle();
            if (!playround(bet, &profit))
                break;
            amount = profit < 0 ? 0UL - (unsigned long)profit : profit;
            printf("Balance: %s$%lu.%s\n", profit < 0 ? "-" : "",
                   amount / 2, amount % 2 ? "50" : "00");
        }
    }
    return 0;
}

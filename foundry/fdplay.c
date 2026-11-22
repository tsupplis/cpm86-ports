#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "conio.h"
#include "foundry.h"

extern int bdos();
extern unsigned fdtick();

static int mono;
static unsigned char shown[HEIGHT][WIDTH];

static void ink(int color)
{
    if (!mono)
        textcolor(0x40 | color); /* Printable VT52 palette byte. */
}

static void tile(int color)
{
    static unsigned char palette[] = {7, 12, 9, 14, 11, 13, 10, 15};
    static char *pattern[] = {"  ", "##", "++", "::", "==", "%%", "**", "oo"};
    ink(palette[color]);
    cputs(!mono && color ? "\333\333" : pattern[color]);
}

static void label(int row, char *text)
{
    gotoxy(row, 29);
    cputs(text);
}

static void frame(void)
{
    int y, x;
    clrscr();
    cursor(CURSOR_OFF);
    ink(7);
    label(2, "F O U N D R Y");
    label(5, "INCOMING");
    label(13, "Left/Right slide   Up rotate");
    label(14, "Down step         SPACE drop");
    label(15, "Q quit           A/D/W/S also work");
    label(17, "Fill whole rows to clear them.");
    label(18, "Clear several at once");
    label(19, "for a higher score.");
    for (y = 0; y <= HEIGHT + 1; ++y) {
        gotoxy(y + 3, 5);
        if (y == 0 || y == HEIGHT + 1) {
            cputc((char)(y == 0 ? 218 : 192));
            for (x = 0; x < 2 * WIDTH; ++x)
                cputc((char)196);
            cputc((char)(y == 0 ? 191 : 217));
        } else {
            cputc((char)179);
            gotoxy(y + 3, 6 + 2 * WIDTH);
            cputc((char)179);
        }
    }
    memset(shown, 0, sizeof(shown));
}

static void render(struct field *f)
{
    unsigned char view[HEIGHT][WIDTH];
    int x, y, i;
    memcpy(view, f->cell, sizeof(view));
    for (i = 0; i < cells(f->active.kind); ++i) {
        offset(f->active.kind, f->active.angle, i, &x, &y);
        view[f->y + y][f->x + x] = f->active.kind + 1;
    }
    for (y = 0; y < HEIGHT; ++y)
        for (x = 0; x < WIDTH; ++x)
            if (shown[y][x] != view[y][x]) {
                gotoxy(y + 4, 6 + 2 * x);
                tile(view[y][x]);
                shown[y][x] = view[y][x];
            }
    ink(7);
}

static void status(struct piece *next, unsigned long score, unsigned long lines)
{
    int i, x, y;
    char number[12];
    for (y = 7; y < 10; ++y)
        label(y, "        ");
    for (i = 0; i < cells(next->kind); ++i) {
        offset(next->kind, next->angle, i, &x, &y);
        gotoxy(7 + y, 29 + 2 * x);
        tile(next->kind + 1);
    }
    ink(7);
    label(10, "Score ");
    sprintf(number, "%10lu", score);
    cputs(number);
    label(11, "Lines ");
    sprintf(number, "%10lu", lines);
    cputs(number);
}

static void newpiece(struct piece *p)
{
    p->angle = 0;
    p->kind = rand() % SHAPES;
}

/* An arrow can arrive over several nonblocking polls. Hold ESC briefly
 * so it can also be used on its own to quit. */
static int cursor_key(int key, int *prefix)
{
    if (*prefix && key > 0) {
        if (*prefix == 1 && (key == '[' || key == 'O')) {
            *prefix = 2;
            return 0;
        }
        *prefix = 0;
        switch (key) {
        case 'A': return 'w';
        case 'B': return 's';
        case 'C': return 'd';
        case 'D': return 'a';
        case '\r': case '\n': return 27;
        default: return key;
        }
    }
    if (key == 27) {
        *prefix = 1;
        return 0;
    }
    return key;
}

int main(int argc, char **argv)
{
    static struct field f;
    struct piece next;
    int i, key, stepped = 0, version, done = 0;
    int elapsed = 0, fall, count;
    int prefix = 0, escape_age = 0, delta, interval, waiting;
    unsigned tick = 0, now;
    unsigned long score = 0, lines = 0;
    char summary[48];

    for (i = 1; i < argc; ++i)
        if (!strcmp(argv[i], "-M"))
            mono = 1;
        else if (!strcmp(argv[i], "-S"))
            stepped = 1;
        else {
            puts("Usage: FOUNDRY [-M] [-S]");
            return 1;
        }
    version = bdos(12, 0) & 255;
    if (!stepped && version != 0x22 && version != 0x31 && version != 0x41) {
        puts("Timed play requires IBM PC CP/M-86, CCP/M 3.1 or CDOS 4.1.");
        return 1;
    }
    interval = version == 0x22 ? 12 : 40;
    srand((unsigned)time((time_t *)0));
    newpiece(&next);
    spawn(&f, &next);
    newpiece(&next);
    conout_direct(!stepped && version == 0x22);
    frame();
    status(&next, score, lines);
    if (!stepped && version == 0x22)
        tick = fdtick();

    while (!done) {
        render(&f);
        if (stepped) {
            gotoxy(22, 0);
            do {
                key = getchar();
            } while (!prefix && (key == '\r' || key == '\n'));
        } else
            key = kbhit() ? getch() : 0;
        waiting = prefix && key == 0;
        if (key)
            escape_age = 0;
        key = cursor_key(key, &prefix);
        if (key == EOF || key == 'Q' || key == 'q' || key == 27 || key == 3)
            break;

        fall = key == 'S' || key == 's';
        switch (key) {
        case 'A': case 'a': move(&f, -1, 0, 0); break;
        case 'D': case 'd': move(&f, 1, 0, 0); break;
        case 'W': case 'w': move(&f, 0, 0, 1); break;
        case ' ':
            while (move(&f, 0, 1, 0))
                ;
            fall = 1;
            break;
        }
        if (!stepped) {
            if (version == 0x22) {
                now = fdtick();
                delta = (unsigned)(now - tick) > 12 ? 12 : (unsigned)(now - tick);
                tick = now;
            } else {
                bdos(141, 2);
                delta = 2;
            }
            elapsed += delta;
            fall |= elapsed >= interval;
            if (waiting) {
                escape_age += delta;
                if (escape_age >= (version == 0x22 ? 4 : 12))
                    break;
            }
        }
        if (!fall)
            continue;
        elapsed = 0;
        if (move(&f, 0, 1, 0))
            continue;
        lock(&f);
        count = clearrows(&f);
        lines += count;
        score += 100UL * count * count;
        done = !spawn(&f, &next);
        newpiece(&next);
        status(&next, score, lines);
    }

    ink(7);
    gotoxy(22, 0);
    cursor(CURSOR_ON);
    sprintf(summary, "%s  Score %lu.\r\n",
            done ? "Foundry full." : "Foundry closed.", score);
    cputs(summary);
    conout_direct(0);
    return 0;
}

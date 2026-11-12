/* Exercise the real buffered renderer and integer formatter under emu2. */
#include "rogue.h"

fighter rogue;
void md_cbreak_no_echo_nonl(boolean on) { }
void md_tstp(void) { }
void md_slurp(void) { }
void start_window(void) { }

static void frame(int n)
{
    printf("\036%d\037", n);
    fflush(stdout);
}

static void draw_symbols(void)
{
    char *symbols = ".A@!-|+?*:/])=,";
    int i;
    for (i = 0; symbols[i]; ++i) mvaddch(4, 10+i, symbols[i]);
}

int main(void)
{
    int saved;
    rogue.fchar = '@';
    initscr();
    draw_symbols();
    mvaddstr(0, 0, "A hero @ . - !");
    mvaddstr(23, 0, "STATUS A @ !");
    move(4, 8); refresh(); frame(1);

    /* A color-only change of the same character must repaint the cell. */
    mvaddstr(4, 11, "A"); refresh(); frame(2);
    mvaddch(4, 11, 'A'); move(4, 9); refresh(); frame(3);

    /* Short rightward cursor movement reprints existing cells. */
    move(4, 14); refresh(); frame(4);
    standout(); mvaddch(4, 14, '-'); standend(); refresh(); frame(5);
    move(4, 12); clrtoeol(); refresh(); frame(6);

    draw_symbols(); wrefresh(curscr); frame(7);
    mvaddstr(4, 11, "A"); refresh(); frame(8);
    mvaddch(4, 11, 'A'); refresh();

    /* Inventory/help saves characters with mvinch and restores with addch. */
    saved = mvinch(4, 11);
    mvaddstr(4, 11, "Z"); refresh(); frame(9);
    mvaddch(4, 11, saved); refresh(); frame(10);
    clear(); refresh(); frame(11);
    standout(); mvaddch(4, 10, '@'); refresh();
    endwin(); frame(12);
    return 0;
}

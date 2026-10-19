/* Sokoban-specific key mapping and coordinate convention.
   Console I/O itself is provided by cpm86-hacking's util.lib. */
#include <stdio.h>
#include "conio.h"
#include "util.h"
#include "gameio.h"

static int concurrent_keys;

void game_keys_init(void)
{
    int version = bdos(12, 0) & 255;
    concurrent_keys = (version >= 0x30 && (ostype() & 5));
    if (concurrent_keys) cputs("\0336");
}

void game_keys_exit(void)
{
    if (concurrent_keys) cputs("\0337");
    concurrent_keys = 0;
}

int game_getkey(void)
{
    int c = getch() & 255;
    if (concurrent_keys) {
        if (c == 0xC8) return 'k';
        if (c == 0xD0) return 'j';
        if (c == 0xCB) return 'h';
        if (c == 0xCD) return 'l';
    }
    return c;
}

/* Sokoban uses column,row; shared conio uses row,column. */
void game_gotoxy(int column, int row)
{
    gotoxy(row, column);
}

#include <stdio.h>
#include "conio.h"
#include "gameio.h"
#include "resource.h"
#include "help.h"

/* Keep the prompt on row 23 of a 24-line display. */
static int help_wait(int more)
{
    int key;
    game_gotoxy(0, 23);
    printf(more ? "Any key: next page   Q/Esc: return to game" :
                  "Any key: return to game");
    fflush(stdout);
    key = game_getkey();
    return key != 'q' && key != 'Q' && key != 27;
}

void showhelp(void)
{
    sk_resource resource;
    char line[79];
    int c, length, rows = 0, last, divider;

    clrscr();
    if (!sk_resource_open(&resource, SK_HELP)) {
        puts("Help is unavailable (missing or invalid resource).");
        help_wait(0);
        return;
    }
    for (;;) {
        length = 0;
        while ((c = sk_resource_getc(&resource)) != EOF && c != '\n') {
            if (c != '\r' && length < sizeof(line)-1) line[length++] = c;
        }
        line[length] = 0;
        last = line[0] == '=' && line[1] == '=';
        divider = line[0] == '+' && line[1] == '=';
        if (last || divider || (c == EOF && !length)) {
            if (rows && !help_wait(!last && c != EOF)) break;
            if (last || c == EOF) break;
            clrscr();
            rows = 0;
            continue;
        }
        if (rows == 22) {
            if (!help_wait(1)) break;
            clrscr();
            rows = 0;
        }
        puts(line);
        ++rows;
        if (c == EOF) {
            help_wait(0);
            break;
        }
    }
    sk_resource_close(&resource);
}

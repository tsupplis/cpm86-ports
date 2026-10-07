/*
  Minimal conio.c for Aztec C on CP/M-86.
  Implements getch(), clrscr(), gotoxy(), textcolor() and cputs()
  using BDOS calls and VT52 terminal escape sequences.

  Adapted from cpm86-hacking conio.c
  (https://github.com/tsupplis/cpm86-hacking).
*/

#include <stdio.h>
#include <stdlib.h>

#include "conio.h"

/* BDOS function numbers */
#define BDOS_WRITEC    2   /* write character to console */
#define BDOS_READC     6   /* direct console I/O */
#define BDOS_READC_BLOCK 0xFF

#define GETCH_BUFLEN (64)

static char *getch_buffer = 0;

/* Output a single character via BDOS */
int cputc(char c)
{
    return bdos(BDOS_WRITEC, c);
}

/* Output a string via BDOS */
void cputs(const char *str)
{
    while (*str) {
        bdos(BDOS_WRITEC, *str++);
    }
}

/* Read a character without echo (blocking), with escape-sequence buffering */
int getch(void)
{
    int i, c, d;
    static int s = 0;
    static int o = 0;

    if (getch_buffer == 0) {
        getch_buffer = (char *)malloc(GETCH_BUFLEN + 1);
    }
    if (s > 0) {
        c = getch_buffer[o];
        s--;
        o++;
        o = o % GETCH_BUFLEN;
        return c;
    }
    /* block until a character arrives */
    while (!(c = bdos(BDOS_READC, BDOS_READC_BLOCK)))
        continue;
    /* buffer any follow-up bytes (e.g. arrow-key sequences) */
    while (s < GETCH_BUFLEN && (d = bdos(BDOS_READC, BDOS_READC_BLOCK))) {
        getch_buffer[(o + s) % GETCH_BUFLEN] = d;
        s++;
    }
    return c;
}

/* Clear screen (VT52: form-feed) */
void clrscr(void)
{
    cputs("\x1bE");
}

/* Position cursor: x = column (0-based), y = row (0-based) */
void gotoxy(int x, int y)
{
    char msg[5];
    msg[0] = 27;
    msg[1] = 'Y';
    msg[2] = y + 32;
    msg[3] = x + 32;
    msg[4] = 0;
    cputs(msg);
}

/* Set foreground color (VT52-style) */
void textcolor(int fg)
{
    unsigned char msg[8];
    if (fg < 1) {
        return;
    }
    msg[0] = 27; msg[1] = 'j';
    msg[2] = 27; msg[3] = 'b';
    msg[4] = (unsigned char)fg;
    msg[5] = 27; msg[6] = 'k';
    msg[7] = 0;
    cputs((char *)msg);
}

#ifndef _CONIO_H_
#define _CONIO_H_

/*
  Minimal conio.h for Aztec C on CP/M-86.
  Provides getch(), clrscr(), gotoxy(), textcolor() and cputs()
  using BDOS calls and VT52 terminal escape sequences.

  Based on the conio implementation in cpm86-hacking
  (https://github.com/tsupplis/cpm86-hacking).
*/

#ifndef __STDC__
int getch();
void clrscr();
void gotoxy(int x, int y);
void textcolor(int fg);
void cputs(char *str);
#else
int getch(void);
void clrscr(void);
void gotoxy(int x, int y);
void textcolor(int fg);
void cputs(const char *str);
#endif

#endif

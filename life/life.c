/*==========================================================================

  life.c

  Simple implementation of Conway's "Game of Life" for CP/M on the
  Z80 Playground. This program uses VT100 or VT52 character codes,
  on an 80x24 terminal.

  Originally written by Kevin Boone, May 2021. No rights reserved.

  Ported to Aztec C86 / CP/M-86 by removing C99/z88dk-isms:
    - // comments replaced by block comments
    - for-init declarations hoisted
    - Z80 inline asm (getrnd) replaced by time(0)-seeded rand()
    - bdos() replaced by putchar() / kbhit() / getch()
    - \xNN escapes replaced by explicit escape characters

==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Outer loop count passed to the assembly delay routine.
   Each unit burns ~65535 LOOP iterations; tune to taste. */
#define DELAY 20

/* Size of the grid. */
#define ROWS 20
#define COLS 60

/* Where to position the grid on the display. */
#define ROW_OFFSET 1
#define COL_OFFSET 10

/* The character to draw for a cell. */
#define CELL_CHAR 'o'

#define TRUE  1
#define FALSE 0
typedef char BOOL;

#define TERM_ANSI 0
#define TERM_VT52 1

/* Change this to get VT100/ANSI codes. */
int term_type = TERM_VT52;

/* Maintain two copies of the grid -- lastgrid is used to calculate which
   character cells need updating; this minimises terminal output. */
char grid     [ROWS][COLS];
char lastgrid [ROWS][COLS];

/* Lookup table for numbers 0..80 -- avoids slow integer-to-ASCII conversion
   in the inner display loop. */
static char *nums[81] =
  {
  "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
  "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
  "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
  "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
  "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
  "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
  "60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
  "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
  "80"
  };

void term_putchar(c)
  char c;
  {
  putchar(c);
  }

/* Output a number <= 80 from the lookup table. */
void putnum(c)
  int c;
  {
  char *s;
  if (c < 10)
    {
    term_putchar('0' + c);
    }
  else
    {
    s = nums[c];
    term_putchar(s[0]);
    term_putchar(s[1]);
    }
  }

/* Show cursor */
void term_show_cursor()
  {
  if (term_type != TERM_VT52)
    printf("\033[?25h");
  }

/* Hide cursor */
void term_hide_cursor()
  {
  if (term_type != TERM_VT52)
    printf("\033[?25l");
  }

/* Set cursor position starting at 0,0 */
void term_set_cursor(row, col)
  int row, col;
  {
  if (term_type == TERM_VT52)
    printf("\033Y%c%c", ' ' + row, ' ' + col);
  else
    {
    term_putchar(27);
    term_putchar('[');
    putnum(row + 1);
    term_putchar(';');
    putnum(col + 1);
    term_putchar('H');
    }
  }

/* Clear screen and home */
void term_clear()
  {
  term_set_cursor(0, 0);
  if (term_type == TERM_VT52)
    printf("\033J");
  else
    printf("\033[J");
  }

/* Return the last character typed, or zero if there wasn't one. */
int term_peekchar()
  {
  /* Aztec C86: bdos(6, 0xFF) is direct console I/O -- returns 0 if no
     key waiting, otherwise the ASCII code. */
  return bdos(6, 0xFF);
  }

/* Draw the grid, updating only cells that changed since the last draw. */
void life_draw_grid()
  {
  char *_grid     = (char *)grid;
  char *_lastgrid = (char *)lastgrid;
  int i, j, row_offset, offset;
  for (i = 0; i < ROWS; i++)
    {
    row_offset = i * COLS;
    for (j = 0; j < COLS; j++)
      {
      offset = row_offset + j;
      if (_grid[offset] != _lastgrid[offset])
        {
        term_set_cursor(i + ROW_OFFSET, j + COL_OFFSET);
        if (_grid[offset])
          term_putchar(CELL_CHAR);
        else
          term_putchar(' ');
        }
      }
    }
  memcpy(lastgrid, grid, sizeof(grid));
  }

/* Clear the display and draw the menu line. */
void life_clear_display()
  {
  term_clear();
  term_set_cursor(ROWS + 1 + ROW_OFFSET, 0);
  printf("                                [R]estart [Q]uit");
  fflush(stdout);
  }

/* Calculate which cells live and which die.  Returns number of changed cells. */
int life_evolve()
  {
  char *_grid     = (char *)grid;
  char *_lastgrid = (char *)lastgrid;
  int i, j, row_offset, offset, c, row_above, row_below, diffs;
  diffs = 0;
  for (i = 0; i < ROWS; i++)
    {
    row_offset = i * COLS;
    for (j = 0; j < COLS; j++)
      {
      offset = row_offset + j;
      c = 0;
      if (i > 0)
        {
        row_above = i - 1;
        if (j > 0        && grid[row_above][j - 1]) c++;
        if (                grid[row_above][j])      c++;
        if (j < COLS - 1 && grid[row_above][j + 1]) c++;
        }
      if (j > 0        && _grid[offset - 1]) c++;
      if (j < COLS - 1 && _grid[offset + 1]) c++;
      if (i < ROWS - 1)
        {
        row_below = i + 1;
        if (j > 0        && grid[row_below][j - 1]) c++;
        if (                grid[row_below][j])      c++;
        if (j < COLS - 1 && grid[row_below][j + 1]) c++;
        }

      if (_grid[offset])
        {
        if (c != 2 && c != 3)
          _grid[offset] = 0;
        }
      else
        {
        if (c == 3) _grid[offset] = 1;
        }
      if (_grid[offset] != _lastgrid[offset]) diffs++;
      }
    }
  return diffs;
  }

/* Assembly busy-wait delay -- burns DELAY * 65535 LOOP iterations. */
static void delay(n)
  unsigned int n;
  {
#include "lifedly.asm"
  }

/* Seed the grid with random live cells. */
void life_init_grid()
  {
  int i, r, c;
  memset(grid,     0, sizeof(grid));
  memset(lastgrid, 0, sizeof(lastgrid));
  for (i = 0; i < 200; i++)
    {
    r = rand() % ROWS;
    c = rand() % COLS;
    grid[r][c] = 1;
    }
  memcpy(lastgrid, grid, sizeof(grid));
  }

int main(argc, argv)
  int argc;
  char **argv;
  {
  BOOL stop = FALSE;
  unsigned int cycles = 0;
  int diffs, c;

  srand((unsigned int)bdos(0x0C, 0));
  life_init_grid();
  life_clear_display();
  term_hide_cursor();
  while (!stop)
    {
    life_draw_grid();
    delay(DELAY);
    term_set_cursor(ROWS + 1, 0);
    diffs = life_evolve();
    if (diffs == 0)
      {
      srand(cycles);
      life_init_grid();
      life_clear_display();
      }
    c = term_peekchar();
    c = tolower(c);
    if (c == 'q') stop = TRUE;
    if (c == 'r')
      {
      srand(cycles);
      life_init_grid();
      life_clear_display();
      }
    cycles++;
    }

  term_clear();
  term_show_cursor();
  fflush(stdout);
  return 0;
  }

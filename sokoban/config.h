#ifndef _CONFIG_H_
#define _CONFIG_H_

#if defined(AZTEC)
#define USE_CONIO
#define USE_COLOR
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_RED 4
#define COLOR_YELLOW 6
#define COLOR_WHITE 7
#define sk_getchar() getch()
#define sk_printf printf
#endif

#if !defined(MAX_ROWS)
#define MAX_ROWS 23
#endif

#if !defined(MAX_COLS)
#define MAX_COLS 40
#endif

#endif

/*
 * A fairly hacked-up version of sokoban, for CP/M. ported 95/7/28 by Russell
 * Marks
 * 
 * It compiles ok (with the usual warnings :-)) under Hitech C. All the source
 * files were basically cat'ted together to make compliation less awkward.
 * 
 * You'll need to patch the 'clear' and 'move' routines with your
 * machine/terminal's clear screen and cursor move codes. (They're currently
 * set for ZCN.) For very simple terminals, you may even be able to patch the
 * binary directly and not bother recompiling.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "config.h"

#if defined(USE_CONIO) || defined(USE_HTC) || defined(AZTEC)
#include "conio.h"
#endif

#if defined(USE_ANSI)
#define clrscr() printf("\033[2J\033[;H")
#define gotoxy(x, y) printf("\033[%d;%dH", y + 1, x + 1)
#define refresh()
#else
#define refresh()
#endif

void color_blue()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_BLUE);
#endif
#if defined(USE_ANSI)
    printf("\033[34m");
#endif
#endif
}

void color_white()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_WHITE);
#endif
#if defined(USE_ANSI)
    printf("\033[37m");
#endif
#endif
}

void color_green()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_GREEN);
#endif
#if defined(USE_ANSI)
    printf("\033[32m");
#endif
#endif
}

void color_yellow()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_YELLOW);
#endif
#if defined(USE_ANSI)
    printf("\033[33m");
#endif
#endif
}

void color_bright_green()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_BRIGHT_GREEN);
#endif
#if defined(USE_ANSI)
    printf("\033[92m");
#endif
#endif
}

void color_bright_cyan()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_BRIGHT_CYAN);
#endif
#if defined(USE_ANSI)
    printf("\033[96m");
#endif
#endif
}

void color_bright_red()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_BRIGHT_RED);
#endif
#if defined(USE_ANSI)
    printf("\033[91m");
#endif
#endif
}

void color_red()
{
#if defined(USE_COLOR)
#if defined(USE_CONIO)
    textcolor(COLOR_RED);
#endif
#if defined(USE_ANSI)
    printf("\033[31m");
#endif
#endif
}

void bg_green()
{
#if defined(USE_COLOR)
#if defined(USE_ANSI)
    printf("\033[42m");
#endif
#endif
}

void bg_yellow()
{
#if defined(USE_COLOR)
#if defined(USE_ANSI)
    printf("\033[43m");
#endif
#endif
}

void bg_purple()
{
#if defined(USE_COLOR)
#if defined(USE_ANSI)
    printf("\033[45m");
#endif
#endif
}

void bg_red()
{
#if defined(USE_COLOR)
#if defined(USE_ANSI)
    printf("\033[41m");
#endif
#endif
}

void color_default()
{
#if defined(USE_COLOR)
#if defined(USE_ANSI)
    printf("\033[0m");
#endif
#if defined(USE_CONIO)
    textcolor(COLOR_WHITE);
#endif
#endif
}

void bg_default()
{
#if defined(USE_COLOR)
#if defined(USE_ANSI)
    printf("\033[0m");
#endif
#endif
}

int readscreen(void);
int testmove(int action);
int checkcmdline(int argc, char *argv[]);
void domove(int moveaction);
void tmpsave(void);
void tmpreset(void);
void errmess(int ret);
void usage(void);
void helpmessage(void);
void dispmoves(void);
void dispsave(void);
void displevel(void);
void disppushes(void);
void disppackets(void);
void showscreen(void);
void undomove(void);
int gameloop(void);
void mapchar(char c, int i, int j);
void color_blue(void);
void color_white(void);
void color_green(void);
void color_yellow(void);
void color_bright_green(void);
void color_bright_cyan(void);
void color_bright_red(void);
void color_red(void);
void bg_green(void);
void bg_yellow(void);
void bg_purple(void);
void bg_red(void);
void color_default(void);
void bg_default(void);

/**/
/* object_t: this typedef is used for internal and external representation */
/* of objects                                                    */
/**/

typedef void (*color_t)(void);

typedef struct _object_t
{
    char obj_intern;   /* internal representation of the object */
    char obj_display1; /* first  display char for the object */
    char obj_display2; /* second display char for the object */
    color_t bg;
    color_t fg;
} object_t;

object_t *get_obj_adr(char c);

/*
 * You can now alter the definitions below. Attention: Do not alter
 * `obj_intern'. This would cause an error
 */
/* when reading the screenfiles                         */
#if defined(USE_PETSCII)
static object_t player = {'@', 214, 215, bg_yellow, color_white};
static object_t playerstore = {'+', 214, 215, bg_red, color_white};
static object_t store = {'.', '.', '.', bg_red, color_white};
static object_t packet = {'$', 216, 216, bg_purple, color_yellow};
static object_t save = {'*', 193, 193, bg_red, color_yellow};
static object_t ground = {' ', ' ', ' ', bg_default, color_default};
static object_t wall = {'#', 230, 230, bg_green, color_green};
#else
static object_t player = {'@', 233, 233, bg_yellow, color_bright_red};
static object_t playerstore = {'+', 233, 233, bg_red, color_bright_red};
static object_t store = {'.', 249, 249, bg_red, color_bright_cyan};
static object_t packet = {'$', 254, 254, bg_purple, color_yellow};
static object_t save = {'*', 254, 254, bg_red, color_bright_green};
static object_t ground = {' ', ' ', ' ', bg_default, color_default};
static object_t wall = {'#', 219, 219, bg_green, color_green};
#endif

/*************************************************************************
********************** DO NOT CHANGE BELOW THIS LINE *********************
*************************************************************************/

typedef struct _pos_t
{
    int x;
    int y;
} pos_t;

#define E_FOPENSCREEN 1
#define E_PLAYPOS1 2
#define E_ILLCHAR 3
#define E_PLAYPOS2 4
#define E_TOMUCHROWS 5
#define E_TOMUCHCOLS 6
#define E_ENDGAME 7
#define E_NOUSER 9
#define E_FOPENSAVE 10
#define E_WRITESAVE 11
#define E_STATSAVE 12
#define E_READSAVE 13
#define E_ALTERSAVE 14
#define E_SAVED 15
#define E_TOMUCHSE 16
#define E_FOPENSCORE 17
#define E_READSCORE 18
#define E_WRITESCORE 19
#define E_USAGE 20
#define E_ILLPASSWORD 21
#define E_LEVELTOOHIGH 22
#define E_NOSUPER 23
#define E_NOSAVEFILE 24

/* defining the types of move */
#define MOVE 1
#define PUSH 2
#define SAVE 3
#define UNSAVE 4
#define STOREMOVE 5
#define STOREPUSH 6

/* defines for control characters */
#define CNTL_L '\014'
#define CNTL_K '\013'
#define CNTL_H '\010'
#define CNTL_J '\012'
#define CNTL_R '\022'
#define CNTL_U '\025'

static pos_t tpos1;             /* testpos1: 1 pos. over/under/left/right */
static pos_t tpos2;             /* testpos2: 2 pos.  "                    */
static pos_t lastppos;          /* the last player position (for undo)    */
static pos_t last_p1, last_p2;  /* last test positions (for undo)         */
static char lppc, ltp1c, ltp2c; /* the char for the above pos. (for undo) */
static char action, lastaction;

/** For the temporary save **/
static char tmp_map[MAX_ROWS + 1][MAX_COLS + 1];
static int tmp_pushes, tmp_moves, tmp_savepack;
static pos_t tmp_ppos;

int scoring = 1;
int level, packets, savepack, moves, pushes, rows, cols;
int scorelevel, scoremoves, scorepushes;
char map[MAX_ROWS + 1][MAX_COLS + 1];
pos_t ppos;
char username[] = "player", *prgname;

int play()
{
    int c;
    int ret;
    int undolock = 1; /* locked for undo */
    int state = 0;

    showscreen();
    tmpsave();
    ret = 0;
    while (ret == 0)
    {
        c = sk_getchar();
        if (c == 27)
        {
            state = 1;
            continue;
        }
        if (c == 0)
        {
            state = 3;
            continue;
        }
        if (c == '[' && state == 1)
        {
            state = 2;
            continue;
        }
        if (state == 3)
        {
            state = 0;
            switch (c)
            {
            case 'M':
                c = 'l';
                break;
            case 'K':
                c = 'h';
                break;
            case 'P':
                c = 'j';
                break;
            case 'H':
                c = 'k';
                break;
            default:
                continue;
            }
        }
        else if (state == 2 || (state == 1 && c >= 'A' && c <= 'D'))
        {
            /* VT100/ANSI sends ESC [ A..D, VT52 sends ESC A..D */
            state = 0;
            switch (c)
            {
            case 'C':
                c = 'l';
                break;
            case 'D':
                c = 'h';
                break;
            case 'B':
                c = 'j';
                break;
            case 'A':
                c = 'k';
                break;
            default:
                continue;
            }
        }
        else
        {
            state = 0;
            switch (c)
            {
#if defined(__C64__)
            case 0x9D: // LEFT
                c = 'h';
                break;
            case 0x11: //DOWN
                c = 'j';
                break;
            case 0x91: // UP
                c = 'k';
                break;
            case 0x1D: //RIGHT
                c = 'l';
                break;
#endif
            case '8':
                c = 'k';
                break;
            case '2':
                c = 'j';
                break;
            case '4':
                c = 'h';
                break;
            case '6':
                c = 'l';
                break;
            case '5':
                c = 'u';
                break;
            case 'E' & 0x3f:
                c = 'k';
                break;
            case 'S' & 0x3f:
                c = 'h';
                break;
            case 'D' & 0x3f:
                c = 'l';
                break;
            case 'X' & 0x3f:
                c = 'j';
                break;

            default:
                break;
            };
        }

        switch (c)
        {
        case 'q': /* quit the game 					 */
            ret = E_ENDGAME;
            break;
        case CNTL_R: /* refresh the screen 				 */
            clrscr();
            showscreen();
            break;
        case 'c': /* temporary save					 */
        case 's':
            tmpsave();
            break;
        case CNTL_U: /* reset to temporary save 			 */
        case 'r':
            tmpreset();
            undolock = 1;
            showscreen();
            break;
        case 'U': /* undo this level 				 */
            moves = pushes = 0;
            if ((ret = readscreen()) == 0)
            {
                showscreen();
                undolock = 1;
            }
            break;
        case 'u': /* undo last move 				 */
            if (!undolock)
            {
                undomove();
                undolock = 1;
            }
            break;

        case 'k':    /* up 						 */
        case 'K':    /* run up 					 */
        case CNTL_K: /* run up, stop before object 			 */
        case 'j':    /* down 						 */
        case 'J':    /* run down 					 */
        case CNTL_J: /* run down, stop before object 			 */
        case 'l':    /* right 						 */
        case 'L':    /* run right 					 */
        case CNTL_L: /* run right, stop before object 			 */
        case 'h':    /* left 						 */
        case 'H':    /* run left 					 */
        case CNTL_H: /* run left, stop before object 			 */
            do
            {
                if ((action = testmove(c)) != 0)
                {
                    lastaction = action;
                    lastppos.x = ppos.x;
                    lastppos.y = ppos.y;
                    lppc = map[ppos.x][ppos.y];
                    last_p1.x = tpos1.x;
                    last_p1.y = tpos1.y;
                    ltp1c = map[tpos1.x][tpos1.y];
                    last_p2.x = tpos2.x;
                    last_p2.y = tpos2.y;
                    ltp2c = map[tpos2.x][tpos2.y];
                    domove(lastaction);
                    undolock = 0;
                }
            } while ((action != 0) && (!islower(c)) && (packets != savepack));
            break;
        default:
            helpmessage();
            break;
        }
        if ((ret == 0) && (packets == savepack))
        {
            scorelevel = level;
            scoremoves = moves;
            scorepushes = pushes;
            break;
        }
    }
    return (ret);
}

int testmove(int action)
{
    int ret;
    register char tc;
    int stop_at_object;

    if ((stop_at_object = iscntrl(action)))
        action = action + 'A' - 1;
    action = (isupper(action)) ? tolower(action) : action;
    if ((action == 'k') || (action == 'j'))
    {
        tpos1.x = (action == 'k') ? ppos.x - 1 : ppos.x + 1;
        tpos2.x = (action == 'k') ? ppos.x - 2 : ppos.x + 2;
        tpos1.y = tpos2.y = ppos.y;
    }
    else
    {
        tpos1.y = (action == 'h') ? ppos.y - 1 : ppos.y + 1;
        tpos2.y = (action == 'h') ? ppos.y - 2 : ppos.y + 2;
        tpos1.x = tpos2.x = ppos.x;
    }
    tc = map[tpos1.x][tpos1.y];
    if ((tc == packet.obj_intern) || (tc == save.obj_intern))
    {
        if (!stop_at_object)
        {
            if (map[tpos2.x][tpos2.y] == ground.obj_intern)
                ret = (tc == save.obj_intern) ? UNSAVE : PUSH;
            else if (map[tpos2.x][tpos2.y] == store.obj_intern)
                ret = (tc == save.obj_intern) ? STOREPUSH : SAVE;
            else
                ret = 0;
        }
        else
            ret = 0;
    }
    else if (tc == ground.obj_intern)
        ret = MOVE;
    else if (tc == store.obj_intern)
        ret = STOREMOVE;
    else
        ret = 0;
    return ret;
}

void domove(int moveaction)
{
    map[ppos.x][ppos.y] = (map[ppos.x][ppos.y] == player.obj_intern)
                              ? ground.obj_intern
                              : store.obj_intern;
    switch (moveaction)
    {
    case MOVE:
        map[tpos1.x][tpos1.y] = player.obj_intern;
        break;
    case STOREMOVE:
        map[tpos1.x][tpos1.y] = playerstore.obj_intern;
        break;
    case PUSH:
        map[tpos2.x][tpos2.y] = map[tpos1.x][tpos1.y];
        map[tpos1.x][tpos1.y] = player.obj_intern;
        pushes++;
        break;
    case UNSAVE:
        map[tpos2.x][tpos2.y] = packet.obj_intern;
        map[tpos1.x][tpos1.y] = playerstore.obj_intern;
        pushes++;
        savepack--;
        break;
    case SAVE:
        map[tpos2.x][tpos2.y] = save.obj_intern;
        map[tpos1.x][tpos1.y] = player.obj_intern;
        savepack++;
        pushes++;
        break;
    case STOREPUSH:
        map[tpos2.x][tpos2.y] = save.obj_intern;
        map[tpos1.x][tpos1.y] = playerstore.obj_intern;
        pushes++;
        break;
    }
    moves++;
    dispmoves();
    disppushes();
    dispsave();
    mapchar(map[ppos.x][ppos.y], ppos.x, ppos.y);
    mapchar(map[tpos1.x][tpos1.y], tpos1.x, tpos1.y);
    mapchar(map[tpos2.x][tpos2.y], tpos2.x, tpos2.y);
    gotoxy(0, MAX_ROWS + 1);
    refresh();
    ppos.x = tpos1.x;
    ppos.y = tpos1.y;
}

void undomove()
{

    map[lastppos.x][lastppos.y] = lppc;
    map[last_p1.x][last_p1.y] = ltp1c;
    map[last_p2.x][last_p2.y] = ltp2c;
    ppos.x = lastppos.x;
    ppos.y = lastppos.y;
    switch (lastaction)
    {
    case MOVE:
        moves--;
        break;
    case STOREMOVE:
        moves--;
        break;
    case PUSH:
        moves--;
        pushes--;
        break;
    case UNSAVE:
        moves--;
        pushes--;
        savepack++;
        break;
    case SAVE:
        moves--;
        pushes--;
        savepack--;
        break;
    case STOREPUSH:
        moves--;
        pushes--;
        break;
    }
    dispmoves();
    disppushes();
    dispsave();
    mapchar(map[ppos.x][ppos.y], ppos.x, ppos.y);
    mapchar(map[last_p1.x][last_p1.y], last_p1.x, last_p1.y);
    mapchar(map[last_p2.x][last_p2.y], last_p2.x, last_p2.y);
    gotoxy(0, MAX_ROWS + 1);
    refresh();
}

void tmpsave()
{
    int i, j;

    for (i = 0; i < rows; i++)
        for (j = 0; j < cols; j++)
            tmp_map[i][j] = map[i][j];
    tmp_pushes = pushes;
    tmp_moves = moves;
    tmp_savepack = savepack;
    tmp_ppos.x = ppos.x;
    tmp_ppos.y = ppos.y;
}

void tmpreset()
{
    int i, j;

    for (i = 0; i < rows; i++)
        for (j = 0; j < cols; j++)
            map[i][j] = tmp_map[i][j];
    pushes = tmp_pushes;
    moves = tmp_moves;
    savepack = tmp_savepack;
    ppos.x = tmp_ppos.x;
    ppos.y = tmp_ppos.y;
}

int converted_getc(FILE *f)
{
    int c = getc(f);
    if (c == EOF)
    {
        return c;
    }
#if defined(__CBM__)
    if (c == '\r')
    {
        c = '\n';
    }
#endif
#if defined(__APPLE2__)
    c = c & 0x7F;
    if (c == '\r')
    {
        c = '\n';
    }
#endif
    return c;
}

int readscreen()
{
    FILE *screen;
    int j, c, f, ret = 0;

    if ((screen = fopen("soklevls.dat", "r")) == NULL)
    {
        ret = E_FOPENSCREEN;
    }
    else
    {
        if (level > 1)
        {
            for (f = 1; f < level; f++)
            {
                while ((c = converted_getc(screen)) != 12 && c != EOF)
                    ;
                converted_getc(screen); /* get the \n after */
            }
        }
        packets = savepack = rows = j = cols = 0;
        ppos.x = -1;
        ppos.y = -1;
        while ((ret == 0) && ((c = converted_getc(screen)) != 12) && c != EOF)
        {
            if (c == '\n')
            {
                map[rows++][j] = '\0';
                if (rows > MAX_ROWS)
                    ret = E_TOMUCHROWS;
                else
                {
                    if (j > cols)
                        cols = j;
                    j = 0;
                }
            }
            else if ((c == player.obj_intern) || (c == playerstore.obj_intern))
            {
                if (ppos.x != -1)
                    ret = E_PLAYPOS1;
                else
                {
                    ppos.x = rows;
                    ppos.y = j;
                    map[rows][j++] = c;
                    if (j > MAX_COLS)
                        ret = E_TOMUCHCOLS;
                }
            }
            else if ((c == save.obj_intern) || (c == packet.obj_intern) ||
                     (c == wall.obj_intern) || (c == store.obj_intern) ||
                     (c == ground.obj_intern))
            {
                if (c == save.obj_intern)
                {
                    savepack++;
                    packets++;
                }
                if (c == packet.obj_intern)
                    packets++;
                map[rows][j++] = c;
                if (j > MAX_COLS)
                    ret = E_TOMUCHCOLS;
            }
            else
            {
                ret = E_ILLCHAR;
                printf("E: %02X %02X %02X", c, '\n', '\r');
                sk_getchar();
            }
        }
        fclose(screen);
        if ((ret == 0) && (ppos.x == -1))
            ret = E_PLAYPOS2;
    }
    return (ret);
}

/*
 * static int        savedbn; static char        *sfname; static FILE
 * *savefile;
 */

void showscreen()
{

    int i, j;

    clrscr();
    for (i = 0; i < rows; i++)
        for (j = 0; map[i][j] != '\0'; j++)
            mapchar(map[i][j], i, j);
    gotoxy(0, MAX_ROWS);
    color_red();
#if MAX_COLS == 20
    sk_printf("level:      packets:      saved:      \r\nmoves:       pushes:");
#else
    sk_printf("Level:      Packets:      Saved:      Moves:       Pushes:");
#endif
    color_default();
    displevel();
    disppackets();
    dispsave();
    dispmoves();
    disppushes();
    gotoxy(0, MAX_ROWS + 2);
    refresh();
}

void mapchar(char c, int i, int j)
{
    object_t *obj;
    color_t col;
    int offset_row = MAX_ROWS - rows < TOP_MARGIN ? MAX_ROWS - rows : TOP_MARGIN;
    int offset_col = MAX_COLS - cols;

    obj = get_obj_adr(c);

    /* if( obj->reverse) standout(); */
    gotoxy(2 * j + offset_col, i + offset_row);
    col = obj->fg;
    col();
    col = obj->bg;
    col();
    sk_printf("%c%c", obj->obj_display1, obj->obj_display2);
    color_default();
    bg_default();
    /* if( obj->reverse) standend(); */
}

object_t *get_obj_adr(char c)
{
    register object_t *ret;

    if (c == player.obj_intern)
        ret = &player;
    else if (c == playerstore.obj_intern)
        ret = &playerstore;
    else if (c == store.obj_intern)
        ret = &store;
    else if (c == save.obj_intern)
        ret = &save;
    else if (c == packet.obj_intern)
        ret = &packet;
    else if (c == wall.obj_intern)
        ret = &wall;
    else if (c == ground.obj_intern)
        ret = &ground;
    else
        ret = &ground;

    return (ret);
}

void displevel()
{
    gotoxy(7, MAX_ROWS);
    sk_printf("%3d", level);
}

void disppackets()
{
    gotoxy(21, MAX_ROWS);
    sk_printf("%3d", packets);
}

void dispsave()
{
    gotoxy(33, MAX_ROWS);
    sk_printf("%3d", savepack);
}

void dispmoves()
{
    gotoxy(45, MAX_ROWS);
    sk_printf("%5d", moves);
}

void disppushes()
{
    gotoxy(59, MAX_ROWS);
    sk_printf("%5d", pushes);
}

void helpmessage()
{
    gotoxy(0, MAX_ROWS + 2);
    /*sk_printf("Press ? for help.");*/
    refresh();
    gotoxy(0, MAX_ROWS + 2);
    refresh();
}

void showhelp()
{
}

static int optrestore = 0;
static int optlevel = 1;

/* static int userlevel; */

int main(int argc, char *argv[])
{
    int ret;

#if defined(__C64__)
    cbm_k_bsout(0x8E);
#endif
#if defined(__APPLE2ENH__)
    videomode(VIDEOMODE_40COL);
#endif
    scorelevel = 0;
    moves = pushes = packets = savepack = 0;
    if ((prgname = strrchr(argv[0], '/')) == NULL)
        prgname = argv[0];
    else
        prgname++;

    {
        if ((ret = checkcmdline(argc, argv)) == 0)
        {
            level = optlevel;
        }
    }
    ret = gameloop();
    errmess(ret);
    return ret;
}

int checkcmdline(int argc, char *argv[])
{
    int ret = 0;

    if (argc == 2)
    {
        if ((optlevel = atoi(argv[1])) == 0)
            ret = E_USAGE;
    }
    return (ret);
}

int gameloop()
{

    int ret = 0;

    /* initscr(); cbreak(); noecho(); */
    if (!optrestore)
        ret = readscreen();
    while (ret == 0)
    {
        if ((ret = play()) == 0)
        {
            level++;
            moves = pushes = packets = savepack = 0;
            ret = readscreen();
        }
    }
    clrscr();
    refresh();
    /* nocbreak(); echo(); endwin(); */
    return (ret);
}

char *message[] = {
    "illegal error number",
    "cannot open screen file",
    "more than one player position in screen file",
    "illegal char in screen file",
    "no player position in screenfile",
    "too much rows in screen file",
    "too much columns in screenfile",
    "quit the game",
    NULL, /* errmessage deleted */
    "cannot get your username",
    "cannot open savefile",
    "error writing to savefile",
    "cannot stat savefile",
    "error reading savefile",
    "cannot restore, your savefile has been altered",
    "game saved",
    "too much users in score table",
    "cannot open score file",
    "error reading scorefile",
    "error writing scorefile",
    "illegal command line syntax",
    "illegal password",
    "level number too big in command line",
    "only superuser is allowed to make a new score table",
    "cannot find file to restore"};

void errmess(int ret)
{
    if (ret != E_ENDGAME)
    {
        fprintf(stderr, "%s: ", prgname);
        switch (ret)
        {
        case E_FOPENSCREEN:
        case E_PLAYPOS1:
        case E_ILLCHAR:
        case E_PLAYPOS2:
        case E_TOMUCHROWS:
        case E_TOMUCHCOLS:
        case E_ENDGAME:
        case E_NOUSER:
        case E_FOPENSAVE:
        case E_WRITESAVE:
        case E_STATSAVE:
        case E_READSAVE:
        case E_ALTERSAVE:
        case E_SAVED:
        case E_TOMUCHSE:
        case E_FOPENSCORE:
        case E_READSCORE:
        case E_WRITESCORE:
        case E_USAGE:
        case E_ILLPASSWORD:
        case E_LEVELTOOHIGH:
        case E_NOSUPER:
        case E_NOSAVEFILE:
            fprintf(stderr, "%s\n", message[ret]);
            break;
        default:
            fprintf(stderr, "%s\n", message[0]);
            break;
        }
        if (ret == E_USAGE)
            usage();
    }
}

void usage()
{
    fprintf(stderr, "Usage: %s [start_level]", prgname);
}

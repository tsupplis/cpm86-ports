/*
 * trek.c  -  C port of STARTREK.BAS
 *
 * Build:  aztec42_cc -c -o trek.o trek.c
 *         aztec42_link -o trek.cmd trek.o -lc86 -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */
#define G               12       /* grid size (quadrants and sectors)  */
#define KLINGON_SHIELDS 400     /* initial klingon shield strength     */

/* sector map codes (index into glyphs-1) */
#define EMPTY   1
#define ENT     2
#define KLING   3
#define BASE    4
#define STAR    5

static char glyphs[] = ".\xE8\xD8\x7F*";   /* glyphs[code-1] */

static char *subsystem[6] = {
    "Warp engines",
    "Short range sensors",
    "Long range sensors",
    "Phasers",
    "Photon torpedoes",
    "Galactic records"
};

/* ------------------------------------------------------------------ */
/* Game context                                                        */
/* ------------------------------------------------------------------ */
typedef struct {
    double  damage[6];                      /* subsystem damage timers        */
    double  klingon_row[G];                 /* klingon positions (row)        */
    double  klingon_col[G];                 /* klingon positions (col)        */
    double  klingon_shields[G];             /* klingon shield levels          */
    int     sector[G][G];                   /* current sector map             */
    int     quadrant[G][G];                 /* galaxy quadrant data K*100+B*10+stars */
    int     galaxy_record[G][G];            /* known galaxy map (-1=unknown)  */
    double  stardate;                       /* current stardate               */
    double  stardate_start;                 /* mission start stardate         */
    double  stardate_end;                   /* mission deadline               */
    double  energy;                         /* enterprise energy              */
    double  energy_max;                     /* max / docked-restore energy    */
    double  torps;                          /* photon torpedoes remaining     */
    double  torps_max;                      /* max / docked-restore torpedoes */
    int     quad_row;                       /* current quadrant row (0-7)     */
    int     quad_col;                       /* current quadrant col (0-7)     */
    int     ent_row;                        /* enterprise sector row (0-7)    */
    int     ent_col;                        /* enterprise sector col (0-7)    */
    int     klingons;                       /* klingons in current quadrant   */
    int     bases;                          /* starbases in current quadrant  */
    int     stars;                          /* stars in current quadrant      */
    int     total_klingons;                 /* galaxy-wide klingon count      */
    int     total_bases;                    /* galaxy-wide starbase count     */
    int     initial_klingons;              /* klingons at mission start      */
    char    condition[16];                  /* GREEN/YELLOW/RED/DOCKED        */
} game_context_t;


/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static double rnd()
{
    return rand() / 32768.0;
}

/* distance from klingon i to enterprise */
static double kdist(game_context_t *g,int i)
{
    double dr = g->klingon_row[i] - g->ent_row;
    double dc = g->klingon_col[i] - g->ent_col;
    return sqrt(dr*dr + dc*dc);
}

/* random sector coords 0-7 */
static void rxy(game_context_t *g,int *x, int *y)
{
    *x = (int)(rnd() * 8);
    *y = (int)(rnd() * 8);
}

/* random empty sector */
static void exy(game_context_t *g,int *x, int *y)
{
    do { rxy(g, x, y); } while (g->sector[*x][*y] != EMPTY);
}

/* ------------------------------------------------------------------ */
/* Klingon attack                                                      */
/* ------------------------------------------------------------------ */
static void kattack(game_context_t *g)
{
    int i;
    double h, d;
    if (g->klingons < 1) return;
    if (strcmp(g->condition, "DOCKED") == 0) {
        printf("Starbase protects Enterprise\n");
        return;
    }
    for (i = 0; i < G; i++) {
        if (g->klingon_shields[i] <= 0) continue;
        h = g->klingon_shields[i] * 0.4 * rnd();
        g->klingon_shields[i] -= h;
        d = kdist(g, i);
        if (d < 0.5) d = 0.5;
        h = h / pow(d, 0.4);
        g->energy -= h;
        printf("%.0f unit hit on Enterprise from sector %.0f-%.0f  (%.0f left)\n",
               h, g->klingon_row[i]+1.0, g->klingon_col[i]+1.0, g->energy);
    }
}

/* ------------------------------------------------------------------ */
/* Condition                                                           */
/* ------------------------------------------------------------------ */
static void chkcond(game_context_t *g)
{
    int i, j;
    for (i = g->ent_row-1; i <= g->ent_row+1; i++) {
        for (j = g->ent_col-1; j <= g->ent_col+1; j++) {
            if (i < 0 || i > 7 || j < 0 || j > 7) continue;
            if (g->sector[i][j] == BASE) {
                strcpy(g->condition, "DOCKED");
                g->energy = g->energy_max;
                g->torps  = g->torps_max;
                for (j = 0; j < 6; j++) g->damage[j] = 0;
                return;
            }
        }
    }
    if (g->klingons > 0)              { strcpy(g->condition, "RED");    return; }
    if (g->energy < g->energy_max*0.1) { strcpy(g->condition, "YELLOW"); return; }
    strcpy(g->condition, "GREEN");
}

/* ------------------------------------------------------------------ */
/* Short-range display                                                 */
/* ------------------------------------------------------------------ */
static void srs(game_context_t *g)
{
    int i, j;
    #if defined(__CPM86__)
            printf("%cE",27);
    #endif 
    for (i = 0; i < G; i++) {
        for (j = 0; j < G; j++) {
            if(g->sector[i][j]-1) {
            #if defined(__CPM86__)
                    printf("%cb%c",27,12);
            #endif  
            } else {
            #if defined(__CPM86__)
                    printf("%cb%c",27,2);
            #endif  
            }
            printf("%c ", glyphs[g->sector[i][j]-1]);
        }
        #if defined(__CPM86__)
                printf("%cb%c",27,3);
        #endif
        printf("  ");
        switch (i) {
        case 0: printf("Years             : %.0f",   g->stardate_end - g->stardate); break;
        case 1: printf("Stardate          : %.0f",   g->stardate); break;
        case 2: printf("Condition         : %s",    g->condition); break;
        case 3: printf("Quadrant          : %d-%d",  g->quad_row+1, g->quad_col+1); break;
        case 4: printf("Sector            : %d-%d",  g->ent_row+1,  g->ent_col+1); break;
        case 5: printf("Energy            : %.0f", g->energy); break;
        case 6: printf("Photon torpedoes  : %.0f", g->torps); break;
        case 7: printf("Klingons left     : %d",g->total_klingons); break;
        }
        printf("\n");
        #if defined(__CPM86__)
                printf("%cu",27);
        #endif
    }
    g->galaxy_record[g->quad_row][g->quad_col] = g->quadrant[g->quad_row][g->quad_col];
}

/* ------------------------------------------------------------------ */
/* Enter quadrant (lines 330-410)                                      */
/* ------------------------------------------------------------------ */
static void enter_quad(game_context_t *g)
{
    int i, n, sx, sy;

    if (g->quad_row < 0 || g->quad_row > 7 || g->quad_col < 0 || g->quad_col > 7) {
        g->quad_row = (int)(rnd()*8); g->quad_col = (int)(rnd()*8);
    }

    /* decode quadrant: klingons*100 + bases*10 + stars */
    n         = g->quadrant[g->quad_row][g->quad_col];
    g->klingons = n / 100;
    g->bases    = (n / 10) % 10;
    g->stars    = n % 10;

    /* clear sector map */
    for (i = 0; i < G; i++) {
        int j; for (j = 0; j < G; j++) g->sector[i][j] = EMPTY;
    }

    /* place enterprise */
    rxy(g, &sx, &sy);
    g->ent_row = sx; g->ent_col = sy;
    g->sector[g->ent_row][g->ent_col] = ENT;

    /* place klingons */
    for (i = 0; i < G; i++) {
        g->klingon_shields[i] = 0;
        g->klingon_row[i] = 8; g->klingon_col[i] = 8;
        if (i < g->klingons) {
            exy(g, &sx, &sy);
            g->sector[sx][sy] = KLING;
            g->klingon_shields[i] = KLINGON_SHIELDS;
            g->klingon_row[i] = sx; g->klingon_col[i] = sy;
        }
    }

    /* place starbase */
    if (g->bases > 0) { exy(g, &sx, &sy); g->sector[sx][sy] = BASE; }

    /* place stars */
    for (i = 0; i < g->stars; i++) { exy(g, &sx, &sy); g->sector[sx][sy] = STAR; }

    chkcond(g);
    if (strcmp(g->condition, "DOCKED") != 0) kattack(g);
}

/* ------------------------------------------------------------------ */
/* Galaxy initialisation                                              */
/* ------------------------------------------------------------------ */
static void init_game(game_context_t *g)
{
    int i, j, k, b, stars;
    double n;

    for (i = 0; i < G; i++)
        for (j = 0; j < G; j++) g->galaxy_record[i][j] = -1;

    g->stardate_start = 3421; g->stardate = g->stardate_start;
    g->energy_max = 4000;     g->energy   = g->energy_max;
    g->torps_max  = 10;       g->torps    = g->torps_max;
    g->total_klingons = 0;    g->total_bases = 0;
    g->stardate_end = 3451;

    /* lines 250-260: build galaxy
       In MBASIC boolean TRUE=-1, FALSE=0.
       Line 250: K=(N<Y1)-Y = (-1 or 0) - 1, so k in [-2,-1]
       Line 230: adds up to 6 more MBASIC booleans (-1 each), so k in [-8,0]
       total_klingons = total_klingons - k  with k<=0  => grows positively.
       total_bases    = total_bases - b     with b in {0,1} => goes negative, flip needed.
       quadrant stored as (-k)*100 + b*10 + stars, all positive. */
    for (i = 0; i < G; i++) {
        for (j = 0; j < G; j++) {
            k = 0;
            n = rnd();
            if (n < 0.2075) {
                n = n * 64.0;
                k = (n < 6.28 ? -1 : 0) - 1;
                if (n < 3.28) k--;
                if (n < 1.8)  k--;
                if (n < 0.28) k--;
                if (n < 0.08) k--;
                if (n < 0.03) k--;
                if (n < 0.01) k--;
                g->total_klingons = g->total_klingons - k;
            }
            b = (rnd() > 0.96) ? 1 : 0;
            g->total_bases = g->total_bases - b;
            stars = (int)(rnd() * 8.0) + 1;
            g->quadrant[i][j] = (-k) * 100 + b * 10 + stars;
        }
    }
    g->total_bases = -g->total_bases;

    if (g->total_klingons > (int)(g->stardate_end - g->stardate_start))
        g->stardate_end = g->stardate_start + g->total_klingons;

    if (g->total_bases < 1) {
        int x, y;
        rxy(g, &x, &y);
        g->quadrant[x][y] += 10;
        g->total_bases = 1;
    }

    g->initial_klingons = g->total_klingons;

    printf("A new StarTrek adventure ...\n");
    printf("Objective: destroy %d Klingon battle cruisers in %d years.\n",
           g->total_klingons, (int)(g->stardate_end - g->stardate_start));
    printf("The number of starbases is %d.\n", g->total_bases);
}

/* ------------------------------------------------------------------ */
/* Print 3-digit zero-padded int                                      */
/* ------------------------------------------------------------------ */
static void p3(int v)
{
    if (v < 0) v = -v;
    v = v % 1000;
    printf("%d%d%d", v/100, (v/10)%10, v%10);
}

/* ------------------------------------------------------------------ */
/* Long range sensors                                                 */
/* ------------------------------------------------------------------ */
static void lrs(game_context_t *g)
{
    int i, j;
    if (g->damage[2] > 0) {
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[2], g->damage[2]);
        return;
    }
    printf("%s for quadrant %d-%d\n", subsystem[2], g->quad_row+1, g->quad_col+1);
    for (i = g->quad_row-1; i <= g->quad_row+1; i++) {
        printf("   ");
        for (j = g->quad_col-1; j <= g->quad_col+1; j++) {
            if (i < 0 || i > 7 || j < 0 || j > 7) {
                printf("***");
            } else {
                g->galaxy_record[i][j] = g->quadrant[i][j];
                p3(g->quadrant[i][j]);
            }
            printf(" ");
        }
        printf("\n");
    }
}

/* ------------------------------------------------------------------ */
/* Galactic records                                                   */
/* ------------------------------------------------------------------ */
static void galactic(game_context_t *g)
{
    int i, j;
    if (g->damage[5] > 0) {
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[5], g->damage[5]);
        return;
    }
    printf("Cumulative galactic map for stardate %.0f\n", g->stardate);
    for (i = 0; i < G; i++) {
        for (j = 0; j < G; j++) {
            if (i == g->quad_row && j == g->quad_col) printf(" +");
            else                                     printf("  ");
            if (g->galaxy_record[i][j] < 0) printf("***");
            else                            p3(g->galaxy_record[i][j]);
        }
        printf("\n");
    }
}

/* ------------------------------------------------------------------ */
/* Movement (warp and torpedo)                                        */
/* ------------------------------------------------------------------ */
static void do_move(game_context_t * g, int is_torp, double course, double warp)
{
    double yp, xp, dy, dx, ang;
    int n, i, yr, xr, crossed;

    n  = is_torp ? 15 : (int)(warp * 8);
    yp = g->ent_row + 0.5;
    xp = g->ent_col + 0.5;

    ang = (course - 1.0) * 0.785398;
    dx  =  cos(ang);
    dy  = -sin(ang);

    if (!is_torp) {
        g->energy -= (double)(n + n) - 0.5;
        g->stardate += 1.0;
        g->sector[g->ent_row][g->ent_col] = EMPTY;
    }

    if (is_torp) printf("Track:  ");

    crossed = 0;
    for (i = 1; i <= n; i++) {
        yp += dy; xp += dx;
        yr = (int)yp; xr = (int)xp;

        if (xr < 0 || xr > 7 || yr < 0 || yr > 7) {
            crossed = 1; break;
        }

        if (is_torp) printf("%d-%d  ", yr+1, xr+1);

        if (g->sector[yr][xr] == EMPTY) continue;

        /* hit something */
        printf("\n");
        if (!is_torp) printf("Blocked by ");

        switch (g->sector[yr][xr]) {
        case KLING:
            printf("Klingon");
            if (!is_torp) {
                printf(" at sector %d-%d\n", yr+1, xr+1);
                g->ent_row = (int)(yp-dy); g->ent_col = (int)(xp-dx);
                g->sector[g->ent_row][g->ent_col] = ENT;
                chkcond(g);
                return;
            }
            for (i = 0; i < G; i++)
                if ((int)g->klingon_row[i]==yr && (int)g->klingon_col[i]==xr)
                    g->klingon_shields[i] = 0;
            g->klingons--; g->total_klingons--;
            break;

        case STAR:
            printf("Star");
            if (!is_torp) {
                printf(" at sector %d-%d\n", yr+1, xr+1);
                g->ent_row = (int)(yp-dy); g->ent_col = (int)(xp-dx);
                g->sector[g->ent_row][g->ent_col] = ENT;
                chkcond(g);
                return;
            }
            g->stars--;
            break;

        case BASE:
            printf("Starbase");
            if (!is_torp) {
                printf(" at sector %d-%d\n", yr+1, xr+1);
                g->ent_row = (int)(yp-dy); g->ent_col = (int)(xp-dx);
                g->sector[g->ent_row][g->ent_col] = ENT;
                chkcond(g);
                return;
            }
            g->bases = 2;   /* flag: starbase destroyed */
            break;

        default:
            return;
        }

        /* torpedo destroyed something */
        printf(" destroyed!");
        if (g->bases == 2) {
            g->bases = 0; g->total_bases--;
            if      (g->total_bases > 1) printf(" %d starbases left....", g->total_bases);
            else if (g->total_bases == 1) printf(" 1 starbase left....");
            else                         printf(" No starbases left....");
        }
        printf("\n");
        g->sector[yr][xr] = EMPTY;
        g->quadrant[g->quad_row][g->quad_col] = g->klingons*100 + g->bases*10 + g->stars;
        if (g->total_klingons < 1) return;
        kattack(g);
        chkcond(g);
        return;
    }

    /* nothing hit */
    if (is_torp) {
        printf("\nMissed!\n");
        kattack(g);
        return;
    }

    if (crossed) {
        g->quad_row = (int)(g->quad_row + warp*dy + (g->ent_row+0.5)/8.0);
        g->quad_col = (int)(g->quad_col + warp*dx + (g->ent_col+0.5)/8.0);
        if (g->quad_row < 0 || g->quad_row > 7 || g->quad_col < 0 || g->quad_col > 7) {
            g->quad_row = (int)(rnd()*8); g->quad_col = (int)(rnd()*8);
        }
        enter_quad(g);
    } else {
        g->ent_row = yr; g->ent_col = xr;
        g->sector[g->ent_row][g->ent_col] = ENT;
        chkcond(g);
    }
}

/* ------------------------------------------------------------------ */
/* Space storm / repair                                               */
/* ------------------------------------------------------------------ */
static void warp_events(game_context_t *g)
{
    int i, x;

    if (rnd() > 0.25) goto decay;

    x = (int)(rnd() * 6);
    if (rnd() > 0.5) {
        g->damage[x] += (int)(6.0 - rnd()*5.0);
        printf("** Space storm, %s damage **\n", subsystem[x]);
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[x], g->damage[x]);
        g->damage[x] += 1.0;
    } else {
        int found = -1;
        for (i = x; i <= 5; i++) if (g->damage[i] > 0) { found = i; break; }
        if (found < 0)
            for (i = 0; i <= x; i++) if (g->damage[i] > 0) { found = i; break; }
        if (found >= 0) {
            g->damage[found] = 0.5;
            printf("** Spock used a new repair technique **\n");
        }
    }

decay:
    for (i = 0; i < 6; i++) {
        if (g->damage[i] == 0.0) continue;
        g->damage[i] -= 1.0;
        if (g->damage[i] <= 0.0) {
            g->damage[i] = 0.0;
            printf("%s are fixed!\n", subsystem[i]);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Phasers                                                            */
/* ------------------------------------------------------------------ */
static void phasers(game_context_t *g)
{
    double x, h, d;
    int i;

    if (g->damage[3] > 0) {
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[3], g->damage[3]);
        return;
    }
    printf("Phasers ready: energy units to fire? ");
    if (scanf("%lf", &x) != 1 || x <= 0) return;
    if (x > g->energy) { printf("Only got %.0f\n", g->energy); phasers(g); return; }

    g->energy -= x;
    for (i = 0; i < G; i++) {
        if (g->klingon_shields[i] <= 0) continue;
        d = kdist(g,i);
        if (d < 0.5) d = 0.5;
        h = x / (g->klingons * pow(d, 0.4));
        g->klingon_shields[i] -= h;
        printf("%.0f unit hit on Klingon at sector %.0f-%.0f  (%.0f left)\n",
               h, g->klingon_row[i]+1.0, g->klingon_col[i]+1.0, g->klingon_shields[i]);
        if (g->klingon_shields[i] <= 0) {
            printf("** Klingon destroyed **\n");
            g->klingons--; g->total_klingons--;
            g->sector[(int)g->klingon_row[i]][(int)g->klingon_col[i]] = EMPTY;
            g->quadrant[g->quad_row][g->quad_col] -= 100;
        }
    }
    if (g->total_klingons < 1) return;
    kattack(g);
}

/* ------------------------------------------------------------------ */
/* Torpedo                                                           */
/* ------------------------------------------------------------------ */
static void torpedo(game_context_t *g)
{
    double course;

    if (g->damage[4] > 0) {
        printf("Space crud blocking tubes ");
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[4], g->damage[4]);
        return;
    }
    if (g->torps < 1) { printf("No torpedoes left\n"); return; }

    printf("Torpedo ");
get_tc:
    printf("Course (1-8.9)? ");
    if (scanf("%lf", &course) != 1 || course < 1) return;
    if (course > 9) goto get_tc;

    g->torps -= 1.0;
    do_move(g, 1, course, 0.0);
}

/* ------------------------------------------------------------------ */
/* Warp                                                               */
/* ------------------------------------------------------------------ */
static void warp(game_context_t *g)
{
    double course, w;

get_wc:
    printf("Course (1-8.9)? ");
    if (scanf("%lf", &course) != 1 || course < 1) return;
    if (course > 9) goto get_wc;

    printf("Warp (0-12)? ");
    if (scanf("%lf", &w) != 1 || w <= 0) goto get_wc;

    if (w > 0.2 && g->damage[0] > 0) {
        printf("%s damaged, max is .2\n", subsystem[0]);
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[0], g->damage[0]);
        goto get_wc;
    }

    kattack(g);
    if (g->energy <= 0) return;

    warp_events(g);
    if (g->stardate > g->stardate_end) return;

    do_move(g, 0, course, w);
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char ** argv)
{
    char buf[64];
    int seed, cmd, i;
    static game_context_t g;

restart:
    printf("Please enter a random number? ");
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    seed = (int)(unsigned char)buf[0];
    seed = seed - 11*(seed/11);
    srand((unsigned)seed * 1234567u + 1u);
    for (i = 0; i <= seed; i++) rnd();
    printf("Working...\n");

    { int x, y; rxy(&g,&x, &y); g.quad_row=x; g.quad_col=y; }

    init_game(&g);
    enter_quad(&g);

game_loop:
    if (g.energy <= 0)                  goto lose;
    if (g.stardate > g.stardate_end)    goto lose;
    if (g.total_klingons < 1)           goto win;

    i = 1;
    if (g.damage[i] > 0) {
        printf("%s damaged. %.0f years estimated for repair.\n\n", subsystem[i], g.damage[i]);
    } else {
        srs(&g);
    }

    printf("Command? ");
    do {
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
    } while (buf[0] == '\n' || buf[0] == '\r');
    cmd = buf[0] - '0';

    switch (cmd) {
    case 1: warp(&g);     break;
    case 2:             break;
    case 3: lrs(&g);      break;
    case 4: phasers(&g);  break;
    case 5: torpedo(&g);  break;
    case 6: galactic(&g); break;
    default:
        for (i = 0; i < 6; i++) printf("  %d= %s\n", i+1, subsystem[i]);
        break;
    }

    if (g.total_klingons < 1)        goto win;
    if (g.energy <= 0 || g.stardate > g.stardate_end) goto lose;
    goto game_loop;

lose:
    printf("\nIt is stardate %.0f\n", g.stardate);
    printf("Thanks to your bungling, the Federation will be\n");
    printf("conquered by the remaining %d Klingon cruisers!\n", g.total_klingons);
    printf("You are demoted to cabin boy!\n");
    goto try_again;

win:
    printf("\nIt is stardate %.0f\n", g.stardate);
    printf("The Federation has been saved!\n");
    printf("You are promoted to Admiral!\n");
    {
        double dt = g.stardate - g.stardate_start;
        if (dt < 1.0) dt = 1.0;
        printf("%d Klingons in %d years.  Rating=%d\n",
               g.initial_klingons, (int)dt,
               (int)((double)g.initial_klingons / dt * 1000.0));
    }

try_again:
    printf("Try again? ");
    do {
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
    } while (buf[0] == '\n' || buf[0] == '\r');
    if (buf[0] == 'Y' || buf[0] == 'y') goto restart;
    return 0;
}

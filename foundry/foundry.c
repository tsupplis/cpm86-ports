#include <string.h>
#include "foundry.h"

/* Spawn coordinates are relative to cell 0, the rotation pivot. */
static struct shape {
    int size;
    signed char xy[5][2];
} shapes[SHAPES] = {
    {3, {{0,0}, {1,0}, {0,1}}},                 /* elbow */
    {3, {{0,0}, {1,0}, {2,0}}},                 /* beam */
    {4, {{0,0}, {1,0}, {2,0}, {3,0}}},          /* rail */
    {4, {{0,0}, {1,0}, {0,1}, {1,1}}},          /* box */
    {4, {{0,0}, {1,0}, {1,1}, {2,1}}},          /* stair */
    {5, {{0,0}, {2,0}, {0,1}, {1,1}, {2,1}}},   /* cup */
    {5, {{0,0}, {1,0}, {2,0}, {1,1}, {1,2}}}    /* fork */
};

int cells(int kind)
{
    return shapes[kind].size;
}

void offset(int kind, int angle, int cell, int *x, int *y)
{
    int a = shapes[kind].xy[cell][0], b = shapes[kind].xy[cell][1];
    switch (angle) {
    case 1: *x = -b; *y = a; break;
    case 2: *x = -a; *y = -b; break;
    case 3: *x = b; *y = -a; break;
    default: *x = a; *y = b; break;
    }
}

int move(struct field *f, int dx, int dy, int turn)
{
    int i, x, y;
    int angle = (f->active.angle + turn) & 3;
    for (i = 0; i < cells(f->active.kind); ++i) {
        offset(f->active.kind, angle, i, &x, &y);
        x += f->x + dx;
        y += f->y + dy;
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || f->cell[y][x])
            return 0;
    }
    f->x += dx;
    f->y += dy;
    f->active.angle = angle;
    return 1;
}

int spawn(struct field *f, struct piece *p)
{
    f->active = *p;
    f->x = WIDTH / 2 - 1;
    f->y = 0;
    return move(f, 0, 0, 0);
}

void lock(struct field *f)
{
    int i, x, y;
    for (i = 0; i < cells(f->active.kind); ++i) {
        offset(f->active.kind, f->active.angle, i, &x, &y);
        f->cell[f->y + y][f->x + x] = f->active.kind + 1;
    }
}

int clearrows(struct field *f)
{
    int y, x, dest = HEIGHT - 1;
    for (y = HEIGHT - 1; y >= 0; --y) {
        for (x = 0; x < WIDTH && f->cell[y][x]; ++x)
            ;
        if (x == WIDTH)
            continue;
        if (dest != y)
            memcpy(f->cell[dest], f->cell[y], WIDTH);
        --dest;
    }
    if (dest >= 0)
        memset(f->cell, 0, (dest + 1) * WIDTH);
    return dest + 1;
}

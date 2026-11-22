#include <assert.h>
#include <string.h>
#include "foundry.h"

int main(void)
{
    struct field f;
    struct piece p = {0, 0};
    int x, y, a, b, i, j, kind, angle, size, occupied;

    memset(&f, 0, sizeof(f));
    assert(spawn(&f, &p));
    assert(!move(&f, 0, 0, 3));
    assert(move(&f, 0, 0, 1));
    assert(move(&f, 0, HEIGHT - 2, 0));
    assert(!move(&f, 0, 1, 0));
    lock(&f);
    assert(f.cell[HEIGHT - 2][f.x] == 1);
    assert(f.cell[HEIGHT - 1][f.x] == 1);
    assert(f.cell[HEIGHT - 2][f.x - 1] == 1);
    f.cell[0][WIDTH / 2 - 1] = 4;
    assert(!spawn(&f, &p));

    for (kind = 0; kind < SHAPES; ++kind) {
        memset(&f, 0, sizeof(f));
        p.kind = kind;
        size = cells(kind);
        assert(size >= 3 && size <= 5 && spawn(&f, &p));
        assert(move(&f, 0, 4, 0));
        for (angle = 0; angle < 4; ++angle) {
            assert(f.active.angle == angle);
            for (i = 0; i < size; ++i) {
                offset(kind, angle, i, &x, &y);
                for (j = 0; j < i; ++j) {
                    offset(kind, angle, j, &a, &b);
                    assert(x != a || y != b);
                }
            }
            assert(move(&f, 0, 0, 1));
        }
        while (move(&f, 0, 1, 0))
            ;
        lock(&f);
        occupied = 0;
        for (y = 0; y < HEIGHT; ++y)
            for (x = 0; x < WIDTH; ++x)
                occupied += f.cell[y][x] != 0;
        assert(occupied == size);
    }

    memset(&f, 0, sizeof(f));
    for (x = 0; x < WIDTH; ++x) {
        f.cell[HEIGHT - 1][x] = x % SHAPES + 1;
        f.cell[HEIGHT - 3][x] = 2;
    }
    f.cell[HEIGHT - 2][1] = 3;
    f.cell[HEIGHT - 4][5] = 4;
    assert(clearrows(&f) == 2);
    assert(f.cell[HEIGHT - 1][1] == 3);
    assert(f.cell[HEIGHT - 2][5] == 4 && f.cell[HEIGHT - 1][5] == 0);
    assert(clearrows(&f) == 0);
    memset(&f, 1, sizeof(f.cell));
    assert(clearrows(&f) == HEIGHT);
    assert(clearrows(&f) == 0);
    return 0;
}

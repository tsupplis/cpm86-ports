#ifndef FOUNDRY_H
#define FOUNDRY_H

#define WIDTH 8
#define HEIGHT 14
#define SHAPES 7

struct piece {
    int kind;
    int angle;
};

struct field {
    unsigned char cell[HEIGHT][WIDTH];
    struct piece active;
    int x, y;
};

int cells(int);
void offset(int, int, int, int *, int *);
int spawn(struct field *, struct piece *);
int move(struct field *, int, int, int);
void lock(struct field *);
int clearrows(struct field *);

#endif

#ifndef GAMEIO_H
#define GAMEIO_H
void game_keys_init(void);
void game_keys_exit(void);
int game_getkey(void);
void game_gotoxy(int column, int row);
#endif

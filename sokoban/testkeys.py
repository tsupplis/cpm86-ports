#!/usr/bin/env python3
"""Host-test Sokoban key/coordinate adapters with mocked shared console calls."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parent
source = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int version, pos, count, used, lastrow, lastcolumn;
static unsigned char keys[16], output[32];
static void check(int ok) { if (!ok) { fputs("Sokoban key check failed\n",stderr); exit(1); } }
int ostype(void) { return version >> 8; }
#define getpriority cpm_getpriority
int bdos(int fn, int arg) {
    if (fn == 12) return version & 255;
    if (fn == 2) { check(used < 32); output[used++] = arg; return 0; }
    check(fn == 6 && arg == 255);
    return pos < count ? keys[pos++] : 0;
}
int getch(void) { check(pos<count); return keys[pos++]; }
int cputs(char *s) { while (*s) bdos(2,*s++); return 0; }
void gotoxy(int row,int column) { lastrow=row; lastcolumn=column; }
#include "gameio.c"
int main(void) {
    int i;
    char expected[] = "kjhl";
    version = 0x1431; game_keys_init();
    check(used == 2 && !memcmp(output,"\0336",2));
    keys[0]=0xC8; keys[1]=0xD0; keys[2]=0xCB; keys[3]=0xCD; count=4;
    for (i=0;i<4;i++) check(game_getkey() == expected[i]);
    game_keys_exit(); game_keys_exit();
    check(used == 4 && !memcmp(output,"\0336\0337",4));
    version = 0x1022; used=pos=0;
    game_keys_init(); keys[0]=27; keys[1]='A'; count=2;
    check(game_getkey()==27 && game_getkey()=='A');
    game_keys_exit(); check(used==0);
    pos=0; count=3; keys[0]=0xC8; keys[1]=0xD0; keys[2]=0xFF;
    check(game_getkey()==0xC8 && game_getkey()==0xD0 && game_getkey()==0xFF);
    version=0x1041; game_keys_init(); game_keys_exit(); check(used==0);
    used=0; game_gotoxy(7,3);
    check(lastrow==3 && lastcolumn==7);
    puts("PASS: Sokoban raw keys, shared-console coordinates and PFK restoration");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='sokoban-keys-') as folder:
    work=Path(folder); (work/'keys.c').write_text(source)
    subprocess.run([os.environ.get('CC_HOST','cc'),'-std=gnu89','-fsanitize=address,undefined',
                    '-I'+str(root),str(work/'keys.c'),'-o',str(work/'keys')],check=True)
    subprocess.run([str(work/'keys')],check=True)

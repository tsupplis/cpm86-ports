/* The real game and help code, with deterministic keyboard input only. */
#define game_getkey test_key
#define main sokoban_main
#include "sokoban.c"
#undef main
#include "help.c"

static char *keys;
static int keypos;
static char before[sizeof(map)];

static void need(int ok, char *message)
{
    if (!ok) { printf("FAIL: %s\n", message); exit(1); }
}

int test_key(void)
{
    need(keys && keys[keypos], "unexpected keyboard read");
    return (unsigned char)keys[keypos++];
}

int main(int argc, char **argv)
{
    int i, j, n, x, y, oldpack, oldsaved;
    unsigned hash;
    char sequence[12], direction = 0;
    char *directions = "hjkl";
    sk_resource resource;

    need(argc >= 2, "test mode");
    if (!stricmp(argv[1], "missing")) {
        need(!sk_resource_open(&resource, SK_LEVELS), "invalid group rejected");
        need(!sk_resource_open(&resource, SK_HELP), "invalid help rejected");
        keys = " "; showhelp();
        need(keypos == 1, "missing help dismisses cleanly");
        puts("MISSING PASS");
        return 0;
    }
    if (!stricmp(argv[1], "levels")) {
        need(argc == 3, "level count");
        n = atoi(argv[2]);
        for (level = 1; level <= n; ++level) {
            need(readscreen() == 0, "load level");
            hash = 0;
            for (i = 0; i < rows; ++i) {
                for (j = 0; map[i][j]; ++j) hash = hash*33U + map[i][j];
                hash = hash*33U + '\n';
            }
            printf("LEVEL %d %d %d %d %d %d %d %u\n", level, rows, cols,
                   packets, savepack, ppos.x, ppos.y, hash);
        }
        need(readscreen() == E_PLAYPOS2, "stop past last level");
        puts("LEVELS PASS");
        return 0;
    }
    level = 1;
    need(readscreen() == 0, "load initial level");
    memcpy(before, map, sizeof(map));
    x = ppos.x; y = ppos.y; oldpack = packets; oldsaved = savepack;
    if (!stricmp(argv[1], "undo")) {
        for (i = 0; directions[i]; ++i)
            if (testmove(directions[i]) == MOVE) { direction = directions[i]; break; }
        need(direction != 0, "find legal move");
        sprintf(sequence, "%c?  uq", direction);
        keys = sequence;
    } else {
        moves = 17; pushes = 5;
        keys = !stricmp(argv[1], "escape") ? "?\033q" :
               (!stricmp(argv[1], "quit") ? "?qq" : "?  q");
    }
    need(play() == E_ENDGAME, "return to game and quit");
    need(keypos == strlen(keys), "consume exactly the expected help keys");
    need(!memcmp(before, map, sizeof(map)), "board unchanged");
    need(ppos.x == x && ppos.y == y, "player unchanged");
    need(packets == oldpack && savepack == oldsaved, "box counts unchanged");
    need(moves == (direction ? 0 : 17) && pushes == (direction ? 0 : 5),
         "counters and undo preserved");
    need(tmp_moves == (direction ? 0 : 17) && tmp_pushes == (direction ? 0 : 5),
         "checkpoint counters preserved");
    need(tmp_ppos.x == x && tmp_ppos.y == y, "checkpoint position preserved");
    for (i = 0; i < rows; ++i)
        need(!memcmp(tmp_map[i], before+i*(MAX_COLS+1), cols), "checkpoint board preserved");
    puts("GAME PASS");
    return 0;
}

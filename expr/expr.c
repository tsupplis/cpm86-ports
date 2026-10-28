#include "tinyexpr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


char * fgets(char*, int, FILE*);

static char *readline(const char *prompt) {
    char buf[1024];
    size_t len;
    char *line;

    fprintf(stderr, "%s", prompt);
    line = fgets(buf, sizeof(buf), stdin);
    if (line == NULL && feof(stdin)) {
        return NULL;
    } else if (line == NULL) {
        printf("fgets error %d\n",errno);
        return NULL;
    }

    len = strlen(line);

    if (len < 1)
        return NULL;

    while(len) {
        if (line[len - 1] == '\n' || line[len - 1] == '\r') {
            line[len - 1] = '\0';
            len -= 1;
        } else {
            break;
        }
    }

    line = malloc(len + 1);
    if (!line) return NULL;
    strcpy(line, buf);
    return line;
}

static void add_history(const char *line) {}

static void strlower(char *s)
{
    for (; *s; s++)
        *s = tolower(*s);
}


static int eval(const char *str) {
    int err = 0;
    double r;
    r = te_interp(str, &err);
    if (err != 0) {
        printf("Error at position %i\n", err);
        return -1;
    } else {
        printf("%g\n", r);
        return 0;
    }
}

static void repl() {
    while (1) {
        char *line = readline("> ");
        if (line == NULL) {
            break;
        } else if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
            free(line);
            break;
        }

        if (eval(line) != -1) {
            add_history(line);
        }

        free(line);
    }
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        strlower(argv[2]);
        if (eval(argv[2]) == -1) {
            return 1;
        } else {
            return 0;
        }
    } else if (argc == 1) {
        repl();
        return 0;
    } else {
        printf("Usage: %s\n", argv[0]);
        printf("       %s -e <expression>\n", argv[0]);
        return 1;
    }
}

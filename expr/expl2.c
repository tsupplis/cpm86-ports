#include "tinyexpr.h"
#include <stdio.h>

static void strlower(char *s)
{
    for (; *s; s++)
        *s = tolower(*s);
}


int main(int argc, char *argv[])
{
    double x, y;
    double r;
    int err;
    te_expr *n;
    char *expression;
    te_variable vars[2];

    if (argc < 2) {
        printf("Usage: example2 \"expression\"\n");
        return 0;
    }

    expression = argv[1];
    strlower(expression);
    printf("Evaluating:\n\t%s\n", expression);

    /* This shows an example where the variables
     * x and y are bound at eval-time. */
    vars[0].name="x";
    vars[0].address=&x;
    vars[1].name="y";
    vars[1].address=&y;

    /* This will compile the expression and check for errors. */

    n = te_compile(expression, vars, 2, &err);

    if (n) {
        /* The variables can be changed here, and eval can be called as many
         * times as you like. This is fairly efficient because the parsing has
         * already been done. */
        x = 3; y = 4;
        r = te_eval(n); printf("Result:\n\t%f\n", r);

        te_free(n);
    } else {
        /* Show the user where the error is at. */
        printf("\t%*s^\nError near here", err-1, "");
    }

    return 0;
}

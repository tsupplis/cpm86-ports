%{
#include <stdio.h>
int yylex(void);
int yyparse(void);
void yyerror(char *message);
static char *input;
static int result;
%}
%token INTEGER
%left '+' '-'
%left '*'
%%
start: optional expr { result = $2; } ;
optional: ;
expr: INTEGER
    | expr '+' expr { $$ = $1 + $3; }
    | expr '-' expr { $$ = $1 - $3; }
    | expr '*' expr { $$ = $1 * $3; }
    | '(' expr ')'  { $$ = $2; }
    ;
%%
int yylex(void)
{
    int c;
    while (*input == ' ') ++input;
    if (!*input) return 0;
    c = *input++;
    if (c >= '0' && c <= '9') {
        yylval = c - '0';
        while (*input >= '0' && *input <= '9')
            yylval = yylval * 10 + *input++ - '0';
        return INTEGER;
    }
    return c;
}
void yyerror(char *message) { }
int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    input = argv[1];
    if (yyparse()) puts("REJECT");
    else printf("RESULT %d\n", result);
    return 0;
}

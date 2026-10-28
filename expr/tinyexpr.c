/*
 * TINYEXPR - Tiny recursive descent parser and evaluation engine in C
 *
 * Copyright (c) 2015-2026 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgement in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* AZTEC-C86-PORTABILITY-REVISION: TE-PORT-2026-10-28-R1 */
/* COMPILE TIME OPTIONS */

/* Exponentiation associativity:
For a^b^c = (a^b)^c and -a^b = (-a)^b do nothing.
For a^b^c = a^(b^c) and -a^b = -(a^b) uncomment the next line.*/
/* #define TE_POW_FROM_RIGHT */

/* Logarithms
For log = base 10 log do nothing
For log = natural log uncomment the next line. */
/* #define TE_NAT_LOG */

#include "tinyexpr.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <ctype.h>

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif


enum {
    TOK_NULL = TE_CLOSURE7+1, TOK_ERROR, TOK_END, TOK_SEP,
    TOK_OPEN, TOK_CLOSE, TOK_NUMBER, TOK_VARIABLE, TOK_INFIX
};

/* Binary operator precedence, lowest first. PREC_NONE marks tokens that
   are never a binary operator (e.g. unary "!"). */
enum {
    PREC_NONE, PREC_LIST, PREC_OR, PREC_AND, PREC_EQ,
    PREC_REL, PREC_SUM, PREC_TERM, PREC_POW
};


enum {TE_CONSTANT = 1};


typedef struct state {
    const char *next;
    int type;
    int prec;
    union { double value; const double *bound; const void *function; } data;
    void *context;

    const te_variable *lookup;
    int lookup_len;
    int applied_unary;
    int depth;
} state;


#define TYPE_MASK(TYPE) ((TYPE)&0x0000001F)

#define IS_PURE(TYPE) (((TYPE) & TE_FLAG_PURE) != 0)
#define IS_FUNCTION(TYPE) (((TYPE) & TE_FUNCTION0) != 0)
#define IS_CLOSURE(TYPE) (((TYPE) & TE_CLOSURE0) != 0)
#define ARITY(TYPE) ( ((TYPE) & (TE_FUNCTION0 | TE_CLOSURE0)) ? ((TYPE) & 0x00000007) : 0 )
#define NEW_EXPR2(type, a, b) new_expr2((type), (a), (b))
#define CHECK_NULL(ptr) if ((ptr) == NULL) return NULL
#define CHECK_NULL_FREE(ptr, owner) if ((ptr) == NULL) { te_free(owner); return NULL; }
#define CHECK_NULL_FREE2(ptr, a, b) if ((ptr) == NULL) { te_free(a); te_free(b); return NULL; }

#ifndef TE_MAX_DEPTH
#define TE_MAX_DEPTH 512
#endif

static te_expr *new_expr(const int type) {
    const int count = ARITY(type) + (IS_CLOSURE(type) ? 1 : 0);
    const unsigned int size = (unsigned int)sizeof(te_expr) +
                              (unsigned int)sizeof(void *) * (unsigned int)count;
    te_expr *ret;
    if (size < (unsigned int)sizeof(te_expr)) return NULL;
    ret = (te_expr *)malloc(size);
    if (ret == NULL) return NULL;
    memset(ret, 0, size);
    ret->parameters = (void **)(ret + 1);
    ret->type = type;
    return ret;
}

static te_expr *new_expr2(const int type, te_expr *a, te_expr *b) {
    te_expr *ret = new_expr(type);
    if (ret == NULL) return NULL;
    ret->parameters[0] = a;
    if (ARITY(type) > 1) ret->parameters[1] = b;
    return ret;
}

static te_expr *new_fun(int type, const void *function, te_expr *a, te_expr *b) {
    /* Builds a function node over a (and b, for arity 2), taking
       ownership of both: on any failure they are freed and NULL returned. */
    te_expr *ret;
    if (a == NULL || (ARITY(type) == 2 && b == NULL)) {
        te_free(a);
        te_free(b);
        return NULL;
    }
    ret = new_expr2(type, a, b);
    if (ret == NULL) {
        te_free(a);
        te_free(b);
        return NULL;
    }
    ret->data.function = function;
    return ret;
}

static void te_free_parameters(te_expr *n) {
    int i;
    if (!n) return;
    for (i = 0; i < ARITY(n->type); ++i) te_free((te_expr *)n->parameters[i]);
}


void te_free(te_expr *n) {
    te_free_parameters(n);
    free(n);
}


static double pi(void) {return 3.14159265358979323846;}
static double e(void) {return 2.71828182845904523536;}
static double fac(double a) {
    unsigned int ua, i;
    double result = 1.0;
    if (!(a >= 0.0)) return NAN;
    if (a > UINT_MAX) return INFINITY;
    ua = (unsigned int)a;
    /* A double factorial overflows after only a few hundred iterations. */
    for (i = 2; i <= ua; ++i) {
        if (result > HUGE_VAL / (double)i) return INFINITY;
        result *= (double)i;
        if (i == UINT_MAX) break;
    }
    return result;
}
static double ncr(double n, double r) {
    unsigned int un, ur, i;
    double result = 1.0;
    if (!(n >= 0.0) || !(r >= 0.0) || n < r) return NAN;
    if (n > UINT_MAX || r > UINT_MAX) return INFINITY;
    un = (unsigned int)n;
    ur = (unsigned int)r;
    if (ur > un / 2U) ur = un - ur;
    for (i = 1; i <= ur; ++i) {
        double factor = (double)(un - ur + i);
        result = result * factor / (double)i;
        if (result == INFINITY) return INFINITY;
        if (i == UINT_MAX) break;
    }
    return result;
}
/* fmod() is not in the Aztec m.lib, so the remainder is built from modf(). */
static double te_fmod(double x, double y) {
    double ax, ay, quotient, remainder;

    /* x-x is NaN for infinities on IEEE targets; NaNs are caught directly. */
    if (x != x || y != y || x - x != 0.0 || y == 0.0) return NAN;
    /* C remainder of a finite value by infinity is the original value. */
    if (y - y != 0.0) return x;

    ax = x < 0.0 ? -x : x;
    ay = y < 0.0 ? -y : y;
    modf(ax / ay, &quotient);
    remainder = ax - quotient * ay;
    /* Rounding of ax/ay can leave the remainder just outside [0, ay). */
    if (remainder < 0.0) remainder += ay;
    else if (remainder >= ay) remainder -= ay;
    return x < 0.0 ? -remainder : remainder;
}

static double npr(double n, double r) {return ncr(n, r) * fac(r);}
/* C99 requires pow(x, 0) == 1; the Aztec C86 libm returns 0 for pow(0, 0). */
static double te_pow(double a, double b) {return b == 0.0 ? 1.0 : pow(a, b);}

#ifdef _MSC_VER
#pragma function (ceil)
#pragma function (floor)
#endif

static const te_variable functions[] = {
    /* must be in alphabetical order */
    {"abs", fabs,     TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"acos", acos,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"asin", asin,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"atan", atan,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"atan2", atan2,  TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"ceil", ceil,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"cos", cos,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"cosh", cosh,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"e", e,          TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"exp", exp,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"fac", fac,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"floor", floor,  TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"ln", log,       TE_FUNCTION1 | TE_FLAG_PURE, 0},
#ifdef TE_NAT_LOG
    {"log", log,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
#else
    {"log", log10,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
#endif
    {"log10", log10,  TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"ncr", ncr,      TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"npr", npr,      TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"pi", pi,        TE_FUNCTION0 | TE_FLAG_PURE, 0},
    {"pow", te_pow,   TE_FUNCTION2 | TE_FLAG_PURE, 0},
    {"sin", sin,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"sinh", sinh,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"sqrt", sqrt,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"tan", tan,      TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {"tanh", tanh,    TE_FUNCTION1 | TE_FLAG_PURE, 0},
    {0, 0, 0, 0}
};

static const te_variable *find_builtin(const char *name, int len) {
    int imin = 0, imax = sizeof(functions) / sizeof(te_variable) - 2;

    while (imax >= imin) {
        int i = imin + (imax - imin) / 2;
        int c = strncmp(name, functions[i].name, len);
        if (!c) c = '\0' - functions[i].name[len];
        if (c == 0) return functions + i;
        if (c > 0) imin = i + 1; else imax = i - 1;
    }
    return 0;
}

static const te_variable *find_lookup(const state *s, const char *name, int len) {
    int i;
    for (i = 0; s->lookup && i < s->lookup_len; ++i) {
        const te_variable *var = s->lookup + i;
        if (strncmp(name, var->name, len) == 0 && var->name[len] == '\0') return var;
    }
    return 0;
}


static double add(double a, double b) {return a + b;}
static double sub(double a, double b) {return a - b;}
static double mul(double a, double b) {return a * b;}
static double divide(double a, double b) {return a / b;}
static double negate(double a) {return -a;}
static double comma(double a, double b) {(void)a; return b;}

static double greater(double a, double b) {return a > b;}
static double greater_eq(double a, double b) {return a >= b;}
static double lower(double a, double b) {return a < b;}
static double lower_eq(double a, double b) {return a <= b;}
static double equal(double a, double b) {return a == b;}
static double not_equal(double a, double b) {return a != b;}
static double logical_and(double a, double b) {return a != 0.0 && b != 0.0;}
static double logical_or(double a, double b) {return a != 0.0 || b != 0.0;}
static double logical_not(double a) {return a == 0.0;}
/* Written as a ternary: the Aztec C86 compiler miscompiles "a != 0.0" as a value. */
static double logical_notnot(double a) {return a == 0.0 ? 0.0 : 1.0;}
static double negate_logical_not(double a) {return -(a == 0.0);}
static double negate_logical_notnot(double a) {return -logical_notnot(a);}


/* Powers of ten up to 1e22 are exact doubles, so dividing by one gives a
   correctly rounded literal where multiplying by pow(10, -n) was an ulp off. */
static const double exact_pow10[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};

static double parse_number(state *s) {
    /* Uses te_strtod so the result does not depend on LC_NUMERIC and
       matches the reference used by the tests. Sets s->type. */
    char *end;
    double value = te_strtod(s->next, &end);

    if (end == s->next) {
        /* A lone '.' is not a number. */
        s->type = TOK_ERROR;
        return 0.0;
    }

    s->type = TOK_NUMBER;
    s->next = end;
    return value;
}


static void infix(state *s, const void *function, int prec) {
    s->type = TOK_INFIX;
    s->data.function = function;
    s->prec = prec;
}

static int match(state *s, char c) {
    if (s->next[0] != c) return 0;
    s->next++;
    return 1;
}

static void next_token(state *s) {
    s->type = TOK_NULL;
    s->prec = PREC_NONE;

    do {
        if (!*s->next) {
            s->type = TOK_END;
            return;
        }

        if (isdigit((unsigned char)s->next[0]) || s->next[0] == '.') {
            s->data.value = parse_number(s);
        } else if (isalpha((unsigned char)s->next[0])) {
            const char *start = s->next;
            const te_variable *var;
            while (isalnum((unsigned char)s->next[0]) || s->next[0] == '_') s->next++;

            var = find_lookup(s, start, (int)(s->next - start));
            if (!var) var = find_builtin(start, (int)(s->next - start));

            if (!var) {
                s->type = TOK_ERROR;
            } else if (TYPE_MASK(var->type) == TE_VARIABLE) {
                s->type = TOK_VARIABLE;
                s->data.bound = var->address;
            } else if (IS_FUNCTION(var->type) != IS_CLOSURE(var->type)) {
                if (IS_CLOSURE(var->type)) s->context = var->context;
                s->type = var->type;
                s->data.function = var->address;
            } else {
                /* Not a type the header defines. */
                s->type = TOK_ERROR;
            }
        } else {
            switch (s->next++[0]) {
                case '+': infix(s, add, PREC_SUM); break;
                case '-': infix(s, sub, PREC_SUM); break;
                case '*': infix(s, mul, PREC_TERM); break;
                case '/': infix(s, divide, PREC_TERM); break;
                case '^': infix(s, te_pow, PREC_POW); break;
                case '%': infix(s, te_fmod, PREC_TERM); break;
                case '!': if (match(s, '=')) infix(s, not_equal, PREC_EQ); else infix(s, logical_not, PREC_NONE); break;
                case '=': if (match(s, '=')) infix(s, equal, PREC_EQ); else s->type = TOK_ERROR; break;
                case '<': if (match(s, '=')) infix(s, lower_eq, PREC_REL); else infix(s, lower, PREC_REL); break;
                case '>': if (match(s, '=')) infix(s, greater_eq, PREC_REL); else infix(s, greater, PREC_REL); break;
                case '&': if (match(s, '&')) infix(s, logical_and, PREC_AND); else s->type = TOK_ERROR; break;
                case '|': if (match(s, '|')) infix(s, logical_or, PREC_OR); else s->type = TOK_ERROR; break;
                case '(': s->type = TOK_OPEN; break;
                case ')': s->type = TOK_CLOSE; break;
                case ',': s->type = TOK_SEP; s->data.function = comma; s->prec = PREC_LIST; break;
                case ' ': case '\t': case '\n': case '\r': break;
                default: s->type = TOK_ERROR; break;
            }
        }
    } while (s->type == TOK_NULL);
}


static te_expr *binary(state *s, int prec);
static te_expr *power(state *s);

static te_expr *base(state *s) {
    /* <base>      =    <constant> | <variable> | <function-0> {"(" ")"} | <function-1> <power> | <function-X> "(" <expr> {"," <expr>} ")" | "(" <list> ")" */
    te_expr *ret;
    int arity, i;

    switch (TYPE_MASK(s->type)) {
        case TOK_NUMBER:
            ret = new_expr(TE_CONSTANT);
            CHECK_NULL(ret);

            ret->data.value = s->data.value;
            next_token(s);
            break;

        case TOK_VARIABLE:
            ret = new_expr(TE_VARIABLE);
            CHECK_NULL(ret);

            ret->data.bound = s->data.bound;
            next_token(s);
            break;

        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            arity = ARITY(s->type);

            ret = new_expr(s->type);
            CHECK_NULL(ret);

            ret->data.function = s->data.function;
            if (IS_CLOSURE(s->type)) ret->parameters[arity] = s->context;
            next_token(s);

            if (arity == 1) {
                ret->parameters[0] = power(s);
                CHECK_NULL_FREE(ret->parameters[0], ret);
            } else if (s->type == TOK_OPEN) {
                i = 0;
                next_token(s);
                while (i < arity) {
                    ret->parameters[i] = binary(s, PREC_OR);
                    CHECK_NULL_FREE(ret->parameters[i], ret);
                    i++;
                    if (i < arity) {
                        if (s->type != TOK_SEP) break;
                        next_token(s);
                    }
                }
                if (i != arity || s->type != TOK_CLOSE) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            } else if (arity != 0) {
                /* Parentheses are optional only on a 0-arity function. */
                s->type = TOK_ERROR;
            }
            break;

        case TOK_OPEN:
            next_token(s);
            ret = binary(s, PREC_LIST);
            CHECK_NULL(ret);

            if (s->type != TOK_CLOSE) {
                s->type = TOK_ERROR;
            } else {
                next_token(s);
            }
            break;

        default:
            ret = new_expr(0);
            CHECK_NULL(ret);

            s->type = TOK_ERROR;
            ret->data.value = NAN;
            break;
    }

    return ret;
}


static te_expr *power(state *s) {
    /* <power>     =    {("-" | "+" | "!")} <base> */
    static double (*unary[2][3])(double) = {
        {NULL, logical_not, logical_notnot},
        {negate, negate_logical_not, negate_logical_notnot}
    };
    int sign = 1, logical = 0;
    double (*f)(double);
    te_expr *ret;

    /* A sign after a "!" is dropped, since !(-x) == !x. */
    while (s->type == TOK_INFIX && 
        (s->data.function == (const void *)add || 
        s->data.function == (const void *)sub || 
        s->data.function == (const void *)logical_not)) {
        if (s->data.function == (const void *)logical_not) logical = logical == 1 ? 2 : 1;
        else if (!logical && s->data.function == (const void *)sub) sign = -sign;
        next_token(s);
    }

    f = unary[sign != 1][logical];

    if (s->depth >= TE_MAX_DEPTH) {
        s->type = TOK_ERROR;
        return NULL;
    }
    s->depth++;
    ret = base(s);
    s->depth--;

    if (f) ret = new_fun(TE_FUNCTION1 | TE_FLAG_PURE, f, ret, NULL);

    /* Distinguishes "-1" from "(-1)" for factor(), which must only hoist
       unary operators actually written to the left of a "^". Set last so
       nested parsing inside base() can't clobber it. */
    s->applied_unary = f != NULL;

    return ret;
}


#ifdef TE_POW_FROM_RIGHT
static te_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    te_expr *ret;
    const void *left_function = NULL;
    te_expr *se;
    void **slot = NULL;
    te_expr *p, *insert;
    int p_unary;
    ret = power(s);
    CHECK_NULL(ret);

    if (s->applied_unary && ret->type == (TE_FUNCTION1 | TE_FLAG_PURE)) {
        left_function = ret->data.function;
        se = (te_expr *)ret->parameters[0];
        free(ret);
        ret = se;
    }

    /* Slot where the next exponent inserts, to make "^" go right-to-left.
       A unary operator on an exponent applies to the whole chain after it,
       so the slot moves inside the unary node. */
    while (s->type == TOK_INFIX && s->data.function == te_pow) {
        next_token(s);

        p = power(s);
        CHECK_NULL_FREE(p, ret);
        p_unary = s->applied_unary;

        if (slot) {
            insert = NEW_EXPR2(TE_FUNCTION2 | TE_FLAG_PURE, (te_expr *)*slot, p);
            CHECK_NULL_FREE2(insert, p, ret);

            insert->data.function = te_pow;
            *slot = insert;
            slot = p_unary ? &p->parameters[0] : &insert->parameters[1];
        } else {
            ret = new_fun(TE_FUNCTION2 | TE_FLAG_PURE, te_pow, ret, p);
            CHECK_NULL(ret);
            slot = p_unary ? &p->parameters[0] : &ret->parameters[1];
        }
    }

    if (left_function) {
        ret = new_fun(TE_FUNCTION1 | TE_FLAG_PURE, left_function, ret, NULL);
        CHECK_NULL(ret);
    }

    return ret;
}
#else
static te_expr *factor(state *s) {
    return binary(s, PREC_POW);
}
#endif


static te_expr *operand(state *s, int prec) {
    return prec == PREC_POW ? power(s) :
           prec == PREC_TERM ? factor(s) : binary(s, prec + 1);
}

static te_expr *binary(state *s, int prec) {
    /* Parses one precedence level of left-associative binary operators:
       <list>      =    <or>     {"," <or>}
       <or>        =    <and>    {"||" <and>}
       <and>       =    <eq>     {"&&" <eq>}
       <eq>        =    <rel>    {("==" | "!=") <rel>}
       <rel>       =    <sum>    {("<" | "<=" | ">" | ">=") <sum>}
       <sum>       =    <term>   {("+" | "-") <term>}
       <term>      =    <factor> {("*" | "/" | "%") <factor>}
       <pow>       =    <power>  {"^" <power>}   (only without TE_POW_FROM_RIGHT) */
    te_expr *ret;
    const void *function;
    ret = operand(s, prec);
    CHECK_NULL(ret);

    while ((s->type == TOK_INFIX || s->type == TOK_SEP) && s->prec == prec) {
        function = s->data.function;
        next_token(s);
        ret = new_fun(TE_FUNCTION2 | TE_FLAG_PURE, function, ret, operand(s, prec));
        CHECK_NULL(ret);
    }

    return ret;
}


#define M(e) te_eval((te_expr *)n->parameters[e])


double te_eval(const te_expr *n) {
    if (!n) return NAN;

    switch(TYPE_MASK(n->type)) {
        case TE_CONSTANT: return n->data.value;
        case TE_VARIABLE: return *n->data.bound;

        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
            switch(ARITY(n->type)) {
                case 0: return ((double (*)(void))n->data.function)();
                case 1: return ((double (*)(double))n->data.function)(M(0));
                case 2: return ((double (*)(double, double))n->data.function)(M(0), M(1));
                case 3: return ((double (*)(double, double, double))n->data.function)(M(0), M(1), M(2));
                case 4: return ((double (*)(double, double, double, double))n->data.function)(M(0), M(1), M(2), M(3));
                case 5: return ((double (*)(double, double, double, double, double))n->data.function)(M(0), M(1), M(2), M(3), M(4));
                case 6: return ((double (*)(double, double, double, double, double, double))n->data.function)(M(0), M(1), M(2), M(3), M(4), M(5));
                case 7: return ((double (*)(double, double, double, double, double, double, double))n->data.function)(M(0), M(1), M(2), M(3), M(4), M(5), M(6));
                default: return NAN;
            }

        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            switch(ARITY(n->type)) {
                case 0: return ((double (*)(void *))n->data.function)(n->parameters[0]);
                case 1: return ((double (*)(void *, double))n->data.function)(n->parameters[1], M(0));
                case 2: return ((double (*)(void *, double, double))n->data.function)(n->parameters[2], M(0), M(1));
                case 3: return ((double (*)(void *, double, double, double))n->data.function)(n->parameters[3], M(0), M(1), M(2));
                case 4: return ((double (*)(void *, double, double, double, double))n->data.function)(n->parameters[4], M(0), M(1), M(2), M(3));
                case 5: return ((double (*)(void *, double, double, double, double, double))n->data.function)(n->parameters[5], M(0), M(1), M(2), M(3), M(4));
                case 6: return ((double (*)(void *, double, double, double, double, double, double))n->data.function)(n->parameters[6], M(0), M(1), M(2), M(3), M(4), M(5));
                case 7: return ((double (*)(void *, double, double, double, double, double, double, double))n->data.function)(n->parameters[7], M(0), M(1), M(2), M(3), M(4), M(5), M(6));
                default: return NAN;
            }

        default: return NAN;
    }

}

#undef M

static void optimize(te_expr *n) {
    int arity, known = 1, i;
    /* Only functions flagged as pure can be folded. Constants and variables
       never carry the flag. */
    if (!IS_PURE(n->type)) return;

    arity = ARITY(n->type);
    for (i = 0; i < arity; ++i) {
        optimize((te_expr *)n->parameters[i]);
        if (((te_expr *)n->parameters[i])->type != TE_CONSTANT) known = 0;
    }
    if (known) {
        double value = te_eval(n);
        te_free_parameters(n);
        n->type = TE_CONSTANT;
        n->data.value = value;
    }
}


te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error) {
    state s;
    te_expr *root;
    s.next = expression;
    s.lookup = variables;
    s.lookup_len = var_count;
    s.depth = 0;

    next_token(&s);
    root = binary(&s, PREC_LIST);
    if (root == NULL) {
        if (error) *error = -1;
        return NULL;
    }

    if (s.type != TOK_END) {
        te_free(root);
        if (error) {
            *error = s.next - expression;
            if (*error == 0) *error = 1;
        }
        return NULL;
    }

    optimize(root);
    if (error) *error = 0;
    return root;
}


double te_interp(const char *expression, int *error) {
    te_expr *n = te_compile(expression, 0, 0, error);
    double ret = n ? te_eval(n) : NAN;
    te_free(n);
    return ret;
}

static void pn (const te_expr *n, int depth) {
    int i, arity;
    printf("%*s", depth, "");

    switch(TYPE_MASK(n->type)) {
    case TE_CONSTANT: printf("%f\n", n->data.value); break;
    case TE_VARIABLE: printf("bound %p\n", n->data.bound); break;

    default:
         arity = ARITY(n->type);
         printf("f%d", arity);
         for(i = 0; i < arity; i++) {
             printf(" %p", n->parameters[i]);
         }
         printf("\n");
         for(i = 0; i < arity; i++) {
             pn(n->parameters[i], depth + 1);
         }
         break;
    }
}


void te_print(const te_expr *n) {
    pn(n, 0);
}


#define TE_DECIMAL_EXP_LIMIT 10000
#define TE_SIGNIFICANT_DIGITS 15

static int te_clamp_exp(int value)
{
    if (value > TE_DECIMAL_EXP_LIMIT) return TE_DECIMAL_EXP_LIMIT;
    if (value < -TE_DECIMAL_EXP_LIMIT) return -TE_DECIMAL_EXP_LIMIT;
    return value;
}

static double te_scale10(double value, int exponent)
{
    /* Scaling by the exact powers of ten (1e0..1e22) keeps the common
       cases correctly rounded: one multiply or divide by an exact power
       is a single IEEE operation. Larger exponents are applied in chunks,
       which avoids overflowing an intermediate 10^n. */
    while (exponent > 22) {
        value *= exact_pow10[22];
        exponent -= 22;
    }
    while (exponent < -22) {
        value /= exact_pow10[22];
        exponent += 22;
    }
    if (exponent > 0) return value * exact_pow10[exponent];
    if (exponent < 0) return value / exact_pow10[-exponent];
    return value;
}

double te_strtod(const char *text, char **endptr)
{
    const char *p;
    const char *number_end;
    double value;
    int negative, digits, significant, nonzero;
    int decimal_exp, explicit_exp, exp_negative;

    p = text;
    while (*p != '\0' && isspace((unsigned char)*p)) ++p;

    negative = 0;
    if (*p == '+' || *p == '-') {
        if (*p == '-') negative = 1;
        ++p;
    }

    value = 0.0;
    digits = 0;
    significant = 0;
    nonzero = 0;
    decimal_exp = 0;

    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X') && isxdigit((unsigned char)p[2])) {
        /* Hex integers; "0x" without a digit is parsed as the 0 alone. */
        p += 2;
        while (isxdigit((unsigned char)*p)) {
            char c = *p;
            int d = isdigit((unsigned char)c) ? c - '0' :
                          (c >= 'a' ? c - 'a' + 10 : c - 'A' + 10);
            value = value * 16.0 + d;
            ++p;
        }
        if (endptr != (char **)0) *endptr = (char *)p;
        return negative ? -value : value;
    }

    while (*p >= '0' && *p <= '9') {
        int digit;
        digit = *p - '0';
        ++digits;
        if (nonzero || digit != 0) {
            nonzero = 1;
            if (significant < TE_SIGNIFICANT_DIGITS) {
                value = value * 10.0 + (double)digit;
                ++significant;
            } else {
                decimal_exp = te_clamp_exp(decimal_exp + 1);
            }
        }
        ++p;
    }

    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9') {
            int digit;
            digit = *p - '0';
            ++digits;
            if (!nonzero && digit == 0) {
                decimal_exp = te_clamp_exp(decimal_exp - 1);
            } else {
                nonzero = 1;
                if (significant < TE_SIGNIFICANT_DIGITS) {
                    value = value * 10.0 + (double)digit;
                    ++significant;
                    decimal_exp = te_clamp_exp(decimal_exp - 1);
                }
                /* Fractional digits below the retained precision are ignored. */
            }
            ++p;
        }
    }

    if (digits == 0) {
        if (endptr != (char **)0) *endptr = (char *)text;
        return 0.0;
    }

    number_end = p;
    if (*p == 'e' || *p == 'E') {
        const char *ep;
        int exp_digits;

        ep = p + 1;
        exp_negative = 0;
        if (*ep == '+' || *ep == '-') {
            if (*ep == '-') exp_negative = 1;
            ++ep;
        }

        explicit_exp = 0;
        exp_digits = 0;
        while (*ep >= '0' && *ep <= '9') {
            int digit;
            digit = *ep - '0';
            exp_digits = 1;
            if (explicit_exp < TE_DECIMAL_EXP_LIMIT) {
                if (explicit_exp > (TE_DECIMAL_EXP_LIMIT - digit) / 10)
                    explicit_exp = TE_DECIMAL_EXP_LIMIT;
                else
                    explicit_exp = explicit_exp * 10 + digit;
            }
            ++ep;
        }

        /* An incomplete exponent is not part of the conversion. */
        if (exp_digits) {
            if (exp_negative) explicit_exp = -explicit_exp;
            decimal_exp = te_clamp_exp(decimal_exp + explicit_exp);
            p = ep;
        } else {
            p = number_end;
        }
    }

    if (negative) value = -value;
    value = te_scale10(value, decimal_exp);
    if (endptr != (char **)0) *endptr = (char *)p;
    return value;
}

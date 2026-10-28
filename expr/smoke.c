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

#include "tinyexpr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "minctest.h"


typedef struct {
    const char *expr;
    double answer;
} test_case;

typedef struct {
    const char *expr1;
    const char *expr2;
} test_equ;


/* File-scope tables keep aggregate initialization compatible with Aztec C86. */
static test_case smoke_syntax_errors[] = {
        {"", 1},
        {"1+", 2},
        {"1)", 2},
        {"(1", 2},
        {"1**1", 3},
        {"1*2(+4", 4},
        {"1*2(1+4", 4},
        {"a+5", 1},
        {"_a+5", 1},
        {"#a+5", 1},
        {"1^^5", 3},
        {"1**5", 3},
        {"sin(cos5", 8},
    };

static const char *smoke_nan_exprs[] = {
        "0/0", "1%0", "1%(1%0)", "(1%0)%1", "fac(-1)",
        "ncr(2, 4)", "ncr(-2, 4)", "ncr(2, -4)",
        "npr(2, 4)", "npr(-2, 4)", "npr(2, -4)"
    };

static const char *smoke_inf_exprs[] = {
        "1/0", "log(0)", "pow(2,10000000)", "fac(300)",
        "ncr(300000,100)", "ncr(300000,100)*8",
        "npr(3,2)*ncr(300000,100)"
    };

static test_case smoke_dynamic_cases[] = {
        {"x", 2},
        {"f+x", 7},
        {"x+x", 4},
        {"x+f", 7},
        {"f+f", 10},
        {"f+sum0", 11},
        {"sum0+sum0", 12},
        {"sum0()+sum0", 12},
        {"sum0+sum0()", 12},
        {"sum0()+(0)+sum0()", 12},
        {"sum1 sum0", 12},
        {"sum1(sum0)", 12},
        {"sum1 f", 10},
        {"sum1 x", 4},
        {"sum2 (sum0, x)", 8},
        {"sum3 (sum0, x, 2)", 10},
        {"sum2(2,3)", 5},
        {"sum3(2,3,4)", 9},
        {"sum4(2,3,4,5)", 14},
        {"sum5(2,3,4,5,6)", 20},
        {"sum6(2,3,4,5,6,7)", 27},
        {"sum7(2,3,4,5,6,7,8)", 35},
    };

static double smoke_closure_values[] = {5,6,7,8,9};

static test_case smoke_closure_cases[] = {
        {"c0", 6},
        {"c1 4", 8},
        {"c2 (10, 20)", 30},
    };

static test_case smoke_closure_cell_cases[] = {
        {"cell 0", 5},
        {"cell 1", 6},
        {"cell 0 + cell 1", 11},
        {"cell 1 * cell 3 + cell 4", 57},
    };

static test_case smoke_optimize_cases[] = {
        {"5+5", 10},
        {"pow(2,2)", 4},
        {"sqrt 100", 10},
        {"pi * 2", 6.2832},
    };

static test_case smoke_combinatorics_cases[] = {
            {"fac(0)", 1},
            {"fac(0.2)", 1},
            {"fac(1)", 1},
            {"fac(2)", 2},
            {"fac(3)", 6},
            {"fac(4.8)", 24},
            {"fac(10)", 3628800},

            {"ncr(0,0)", 1},
            {"ncr(10,1)", 10},
            {"ncr(10,0)", 1},
            {"ncr(10,10)", 1},
            {"ncr(16,7)", 11440},
            {"ncr(16,9)", 11440},
            {"ncr(100,95)", 75287520},
            {"ncr(63,29)", 759510004936100355.0},
            {"ncr(63,30)", 860778005594247069.0},
            {"ncr(64,32)", 1832624140942590534.0},
            {"ncr(66,33)", 7219428434016265740.0},

            {"npr(0,0)", 1},
            {"npr(10,1)", 10},
            {"npr(10,0)", 1},
            {"npr(10,10)", 3628800},
            {"npr(20,5)", 1860480},
            {"npr(100,4)", 94109400},
            {"npr(63,3)", 238266},
            {"npr(66,2)", 4290},
    };

static test_case smoke_logic_cases[] = {
            {"1 && 1", 1},
            {"1 && 0", 0},
            {"0 && 1", 0},
            {"0 && 0", 0},
            {"1 || 1", 1},
            {"1 || 0", 1},
            {"0 || 1", 1},
            {"0 || 0", 0},
            {"!0", 1},
            {"!1", 0},
            {"!2", 0},

            {"!-2", 0},
            {"-!2", 0},
            {"!!0", 0},
            {"!!1", 1},
            {"!!2", 1},
            {"!!-2", 1},
            {"!-!2", 1},
            {"-!!2", -1},
            {"--!!2", 1},

            {"1 < 2", 1},
            {"2 < 2", 0},
            {"2 <= 2", 1},
            {"2 > 1", 1},
            {"2 > 2", 0},
            {"2 >= 2", 1},
            {"2 > -2", 1},
            {"-2 < 2", 1},

            {"0 == 0", 1},
            {"0 != 0", 0},
            {"2 == 2", 1},
            {"2 != 2", 0},
            {"2 == 3", 0},
            {"2 != 3", 1},
            {"2 == 2.0001", 0},
            {"2 != 2.0001", 1},

            {"1 < 2 && 2 < 3", 1},
            {"1 < 2 && 3 < 2", 0},
            {"2 < 1 && 2 < 3", 0},
            {"2 < 1 && 3 < 2", 0},
            {"1 < 2 || 2 < 3", 1},
            {"1 < 2 || 3 < 2", 1},
            {"2 < 1 || 2 < 3", 1},
            {"2 < 1 || 3 < 2", 0},

            {"0 == 1 < 2", 0},
            {"1 == 1 < 2", 1},
            {"1 != 2 > 3", 1},
            {"1 || 0 && 0", 1},
            {"1 || 1 && 0", 1},
            {"0 && 0 || 1", 1},
            {"!1 == 0", 1},

            {"1 < 1+1", 1},
            {"1 < 1*2", 1},
            {"1 < 2/2", 0},
            {"1 < 2^2", 1},

            {"5+5 < 4+10", 1},
            {"5+(5 < 4)+10", 15},
            {"5+(5 < 4+10)", 6},
            {"(5+5 < 4)+10", 10},
            {"5+!(5 < 4)+10", 16},
            {"5+!(5 < 4+10)", 5},
            {"!(5+5 < 4)+10", 11},
            {"!0^2", 1},
            {"!0^-1", 1},
            {"-!0^2", 1},
    };

static int smoke_depths[] = {10, 40};

static const char *smoke_partial_number_exprs[] = {"1e", "1e+", "0x", "."};

static const char *smoke_exact_number_exprs[] = {
            "0.3", "0.7", "867.4", "26.06", "522.526", "1.1", "2.675",
            "0.0001", "123456.789", "9.87654321e-5", "4.35e-10", "1.5e-22",
            "0.1", "0.2", "3.14159265358979"
        };


void test_results() {
    int i;
    test_case cases[18];
    cases[0].expr="1";
    cases[0].answer = 1;
    cases[1].expr = "1 ";
    cases[1].answer = 1;
    cases[2].expr = "(1)";
    cases[2].answer = 1;
    cases[3].expr = "pi";
    cases[3].answer = 3.14159;
    cases[4].expr = "atan(1)*4 - pi";
    cases[4].answer = 0;
    cases[5].expr = "e";
    cases[5].answer = 2.71828;

    cases[6].expr = "2+1";
    cases[6].answer = 2+1;
    cases[7].expr = "(((2+(1))))";
    cases[7].answer = 2+1;
    cases[8].expr = "3+2";
    cases[8].answer = 3+2;
    cases[9].expr = "3+2+4";
    cases[9].answer = 3+2+4;
    cases[10].expr = "(3+2)+4";
    cases[10].answer = 3+2+4;
    cases[11].expr = "3+(2+4)";
    cases[11].answer = 3+2+4;
    cases[12].expr = "(3+2+4)";
    cases[12].answer = 3+2+4;   
    cases[13].expr = "3*2*4";
    cases[13].answer = 3*2*4;
    cases[14].expr = "(3*2)*4";
    cases[14].answer = 3*2*4;
    cases[15].expr = "3*(2*4)";
    cases[15].answer = 3*2*4;
    cases[16].expr = "(3*2*4)";
    cases[16].answer = 3*2*4;
    cases[17].expr = "3-2-4";
    cases[17].answer = 3-2-4;
    cases[17].expr = "3-2-4";
    cases[17].answer = 3-2-4;
/**
        {"(3-2)-4", (3-2)-4},
        {"3-(2-4)", 3-(2-4)},
        {"(3-2-4)", 3-2-4},

        {"3/2/4", 3.0/2.0/4.0},
        {"(3/2)/4", (3.0/2.0)/4.0},
        {"3/(2/4)", 3.0/(2.0/4.0)},
        {"(3/2/4)", 3.0/2.0/4.0},

        {"(3*2/4)", 3.0*2.0/4.0},
        {"(3/2*4)", 3.0/2.0*4.0},
        {"3*(2/4)", 3.0*(2.0/4.0)},

        {"asin sin .5", 0.5},
        {"sin asin .5", 0.5},
        {"ln exp .5", 0.5},
        {"exp ln .5", 0.5},

        {"asin sin-.5", -0.5},
        {"asin sin-0.5", -0.5},
        {"asin sin -0.5", -0.5},
        {"asin (sin -0.5)", -0.5},
        {"asin (sin (-0.5))", -0.5},
        {"asin sin (-0.5)", -0.5},
        {"(asin sin (-0.5))", -0.5},

        {"log10 1000", 3},
        {"log10 1e3", 3},
        {"log10 1000", 3},
        {"log10 1e3", 3},
        {"log10(1000)", 3},
        {"log10(1e3)", 3},
        {"log10 1.0e3", 3},
        {"10^5*5e-5", 5},

#ifdef TE_NAT_LOG
        {"log 1000", 6.9078},
        {"log e", 1},
        {"log (e^10)", 10},
#else
        {"log 1000", 3},
#endif

        {"ln (e^10)", 10},
        {"100^.5+1", 11},
        {"100 ^.5+1", 11},
        {"100^+.5+1", 11},
        {"100^--.5+1", 11},
        {"100^---+-++---++-+-+-.5+1", 11},

        {"100^-.5+1", 1.1},
        {"100^---.5+1", 1.1},
        {"100^+---.5+1", 1.1},
        {"1e2^+---.5e0+1e0", 1.1},
        {"--(1e2^(+(-(-(-.5e0))))+1e0)", 1.1},

        {"sqrt 100 + 7", 17},
        {"sqrt 100 * 7", 70},
        {"sqrt (100 * 100)", 100},

        {"1,2", 2},
        {"1,2+1", 3},
        {"1+1,2+2,2+1", 3},
        {"1,2,3", 3},
        {"(1,2),3", 3},
        {"1,(2,3)", 3},
        {"-(1,(2,3))", -3},

        {"2^2", 4},
        {"pow(2,2)", 4},

        {"atan2(1,1)", 0.7854},
        {"atan2(1,2)", 0.4636},
        {"atan2(2,1)", 1.1071},
        {"atan2(3,4)", 0.6435},
        {"atan2(3+3,4*2)", 0.6435},
        {"atan2(3+3,(4*2))", 0.6435},
        {"atan2((3+3),4*2)", 0.6435},
        {"atan2((3+3),(4*2))", 0.6435},
    };

*/
    for (i = 0; i < sizeof(cases) / sizeof(test_case); ++i) {
        double ev;
        int err;
        const char *expr = cases[i].expr;
        const double answer = cases[i].answer;

        ev = te_interp(expr, &err);
        lok(!err);
        lfequal(ev, answer);

        if (err) {
            printf("FAILED: %s (%d)\n", expr, err);
        }
    }
}


/* Aztec C86 has no NaN (0.0/0.0 is 0.0), so there tinyexpr reports errors as 0. */
static int is_error_result(double r) {
    double zero = 0.0;
    if (zero / zero != zero / zero) return r != r;
    return r == 0.0;
}

/* Aztec C86 has no usable infinity (1.0/0.0 is 1.0), so the INF checks are skipped there. */
static int has_inf(void) {
    double one = 1.0, zero = 0.0;
    return one / zero == one / zero + 1.0;
}

void test_syntax() {
    int i, err;
    
    for (i = 0; i < sizeof(smoke_syntax_errors) / sizeof(test_case); ++i) {
        te_expr *n;
        const char *expr;
        int e;
        double r, k;

        expr = smoke_syntax_errors[i].expr;
        e = (int)smoke_syntax_errors[i].answer;
        r = te_interp(expr, &err);
        lequal(err, e);
        lok(is_error_result(r));

        n = te_compile(expr, 0, 0, &err);
        lequal(err, e);
        lok(!n);

        if (err != e) {
            printf("FAILED: %s\n", expr);
        }

        k = te_interp(expr, 0);
        lok(is_error_result(k));
    }
}


void test_nans() {
    int i;
    int err;
    double r, c;
    te_expr *n;
    const char *expr;
    
    for (i = 0; i < sizeof(smoke_nan_exprs) / sizeof(smoke_nan_exprs[0]); ++i) {
        expr = smoke_nan_exprs[i];
        r = te_interp(expr, &err);
        lequal(err, 0);
        lok(is_error_result(r));
        n = te_compile(expr, 0, 0, &err);
        lok(n != 0);
        lequal(err, 0);
        c = te_eval(n);
        lok(is_error_result(c));
        te_free(n);
    }
}


void test_infs() {
    int i;
    int err;
    double r, c;
    te_expr *n;
    const char *expr;

    if (!has_inf()) {
        printf("INFs skipped: no infinity on this platform\n");
        return;
    }
    
    for (i = 0; i < sizeof(smoke_inf_exprs) / sizeof(smoke_inf_exprs[0]); ++i) {
        expr = smoke_inf_exprs[i];
        r = te_interp(expr, &err);
        lequal(err, 0);
        lok(r - r != 0.0);
        n = te_compile(expr, 0, 0, &err);
        lok(n != 0);
        lequal(err, 0);
        c = te_eval(n);
        lok(c - c != 0.0);
        te_free(n);
    }
}

void test_variables() {
    double x, y, test, z;
    int err;
    te_variable lookup[3];
    te_variable bogus[2];
    te_expr *expr1, *expr2, *expr3, *expr4;
    te_expr *expr5, *expr6, *expr7, *expr8, *expr9, *expr10;
    double ev;
    lookup[0].name = "x"; lookup[0].address = &x; lookup[0].type = TE_VARIABLE; lookup[0].context = 0;
    lookup[1].name = "y"; lookup[1].address = &y; lookup[1].type = TE_VARIABLE; lookup[1].context = 0;
    lookup[2].name = "te_st"; lookup[2].address = &test; lookup[2].type = TE_VARIABLE; lookup[2].context = 0;
    expr1 = te_compile("cos x + sin y", lookup, 2, &err);
    lok(expr1); lok(!err);
    expr2 = te_compile("x+x+x-y", lookup, 2, &err);
    lok(expr2); lok(!err);
    expr3 = te_compile("x*y^3", lookup, 2, &err);
    lok(expr3); lok(!err);
    expr4 = te_compile("te_st+5", lookup, 3, &err);
    lok(expr4); lok(!err);
    for (y = 2; y < 3; ++y) {
        for (x = 0; x < 5; ++x) {
            ev = te_eval(expr1); lfequal(ev, cos(x) + sin(y));
            ev = te_eval(expr2); lfequal(ev, x+x+x-y);
            ev = te_eval(expr3); lfequal(ev, x*y*y*y);
            test = x; ev = te_eval(expr4); lfequal(ev, x+5);
        }
    }
    te_free(expr1); te_free(expr2); te_free(expr3); te_free(expr4);
    expr5 = te_compile("xx*y^3", lookup, 2, &err);
    lok(!expr5); lok(err);
    expr6 = te_compile("tes", lookup, 3, &err);
    lok(!expr6); lok(err);
    expr7 = te_compile("sinn x", lookup, 2, &err);
    lok(!expr7); lok(err);
    expr8 = te_compile("si x", lookup, 2, &err);
    lok(!expr8); lok(err);
    /* Invalid lookup types and function/closure conflicts remain errors. */
    z = 1;
    bogus[0].name = "z"; bogus[0].address = &z; bogus[0].type = 7; bogus[0].context = 0;
    bogus[1].name = "w"; bogus[1].address = &z; bogus[1].type = TE_FUNCTION0 | TE_CLOSURE0; bogus[1].context = 0;
    expr9 = te_compile("z+1", bogus, 2, &err);
    lok(!expr9); lok(err);
    expr10 = te_compile("w+1", bogus, 2, &err);
    lok(!expr10); lok(err);
}


#define cross_check(a, b) do {\
    if ((b)!=(b)) break;\
    expr = te_compile((a), lookup, 2, &err);\
    lfequal(te_eval(expr), (b));\
    lok(!err);\
    te_free(expr);\
}while(0)

void test_functions() {
    double x, y;
    te_variable lookup[2];
    int err;
    te_expr *expr;
    lookup[0].name = "x"; lookup[0].address = &x; lookup[0].type = TE_VARIABLE; lookup[0].context = 0;
    lookup[1].name = "y"; lookup[1].address = &y; lookup[1].type = TE_VARIABLE; lookup[1].context = 0;

    for (x = -5; x < 5; x += .2) {
        cross_check("abs x", fabs(x));
        cross_check("acos x", acos(x));
        cross_check("asin x", asin(x));
        cross_check("atan x", atan(x));
        cross_check("ceil x", ceil(x));
        cross_check("cos x", cos(x));
        cross_check("cosh x", cosh(x));
        cross_check("exp x", exp(x));
        cross_check("floor x", floor(x));
        cross_check("ln x", log(x));
        cross_check("log10 x", log10(x));
        cross_check("sin x", sin(x));
        cross_check("sinh x", sinh(x));
        cross_check("sqrt x", sqrt(x));
        cross_check("tan x", tan(x));
        cross_check("tanh x", tanh(x));

        for (y = -2; y < 2; y += .2) {
            if (fabs(x) < 0.01) break;
            cross_check("atan2(x,y)", atan2(x, y));
            cross_check("pow(x,y)", pow(x, y));
        }
    }
}


double sum0() {
    return 6;
}
double sum1(double a) {
    return a * 2;
}
double sum2(double a, double b) {
    return a + b;
}
double sum3(double a, double b, double c) {
    return a + b + c;
}
double sum4(double a, double b, double c, double d) {
    return a + b + c + d;
}
double sum5(double a, double b, double c, double d, double e) {
    return a + b + c + d + e;
}
double sum6(double a, double b, double c, double d, double e, double f) {
    return a + b + c + d + e + f;
}
double sum7(double a, double b, double c, double d, double e, double f, double g) {
    return a + b + c + d + e + f + g;
}


void test_dynamic() {
    double x, f;
    int i, err;
    const char *expr;
    double answer;
    te_expr *ex;
    te_variable lookup[10];
    
    lookup[0].name = "x"; lookup[0].address = &x; lookup[0].type = TE_VARIABLE; lookup[0].context = 0;
    lookup[1].name = "f"; lookup[1].address = &f; lookup[1].type = TE_VARIABLE; lookup[1].context = 0;
    lookup[2].name = "sum0"; lookup[2].address = sum0; lookup[2].type = TE_FUNCTION0; lookup[2].context = 0;
    lookup[3].name = "sum1"; lookup[3].address = sum1; lookup[3].type = TE_FUNCTION1; lookup[3].context = 0;
    lookup[4].name = "sum2"; lookup[4].address = sum2; lookup[4].type = TE_FUNCTION2; lookup[4].context = 0;
    lookup[5].name = "sum3"; lookup[5].address = sum3; lookup[5].type = TE_FUNCTION3; lookup[5].context = 0;
    lookup[6].name = "sum4"; lookup[6].address = sum4; lookup[6].type = TE_FUNCTION4; lookup[6].context = 0;
    lookup[7].name = "sum5"; lookup[7].address = sum5; lookup[7].type = TE_FUNCTION5; lookup[7].context = 0;
    lookup[8].name = "sum6"; lookup[8].address = sum6; lookup[8].type = TE_FUNCTION6; lookup[8].context = 0;
    lookup[9].name = "sum7"; lookup[9].address = sum7; lookup[9].type = TE_FUNCTION7; lookup[9].context = 0;


    x = 2;
    f = 5;

    for (i = 0; i < sizeof(smoke_dynamic_cases) / sizeof(test_case); ++i) {
        expr = smoke_dynamic_cases[i].expr;
        answer = smoke_dynamic_cases[i].answer;
        ex = te_compile(expr, lookup, sizeof(lookup)/sizeof(te_variable), &err);
        lok(ex);
        lfequal(te_eval(ex), answer);
        te_free(ex);
    }
}


double clo0(void *context) {
    if (context) return *((double*)context) + 6;
    return 6;
}
double clo1(void *context, double a) {
    if (context) return *((double*)context) + a * 2;
    return a * 2;
}
double clo2(void *context, double a, double b) {
    if (context) return *((double*)context) + a + b;
    return a + b;
}

double cell(void *context, double a) {
    double *c = context;
    return c[(int)a];
}

void test_closure() {
    double extra;
    int i, err;
    const char *expr;
    double answer;
    te_expr *ex;
    

    
    
    te_variable lookup[4];
    lookup[0].name = "c0"; lookup[0].address = clo0; lookup[0].type = TE_CLOSURE0; lookup[0].context = &extra;
    lookup[1].name = "c1"; lookup[1].address = clo1; lookup[1].type = TE_CLOSURE1; lookup[1].context = &extra;
    lookup[2].name = "c2"; lookup[2].address = clo2; lookup[2].type = TE_CLOSURE2; lookup[2].context = &extra;
    lookup[3].name = "cell"; lookup[3].address = cell; lookup[3].type = TE_CLOSURE1; lookup[3].context = smoke_closure_values;


    for (i = 0; i < sizeof(smoke_closure_cases) / sizeof(test_case); ++i) {
        expr = smoke_closure_cases[i].expr;
        answer = smoke_closure_cases[i].answer;
        ex = te_compile(expr, lookup, sizeof(lookup)/sizeof(te_variable), &err);
        lok(ex);

        extra = 0;
        lfequal(te_eval(ex), answer + extra);

        extra = 10;
        lfequal(te_eval(ex), answer + extra);

        te_free(ex);
    }


    for (i = 0; i < sizeof(smoke_closure_cell_cases) / sizeof(test_case); ++i) {
        expr = smoke_closure_cell_cases[i].expr;
        answer = smoke_closure_cell_cases[i].answer;
        ex = te_compile(expr, lookup, sizeof(lookup)/sizeof(te_variable), &err);
        lok(ex);
        lfequal(te_eval(ex), answer);
        te_free(ex);
    }
}

void test_optimize() {

    

    int i;
    for (i = 0; i < sizeof(smoke_optimize_cases) / sizeof(test_case); ++i) {
        const char *expr = smoke_optimize_cases[i].expr;
        const double answer = smoke_optimize_cases[i].answer;

        int err;
        te_expr *ex = te_compile(expr, 0, 0, &err);
        lok(ex);
        if (!ex) continue;

        /* The answer should be know without
         * even running eval. */
        lfequal(ex->data.value, answer);
        lfequal(te_eval(ex), answer);

        te_free(ex);
    }
}

static test_equ te_pow_cases_left[] = {
    {"2^3^4", "(2^3)^4"},
    {"-2^2", "(-2)^2"},
    {"(-2)^2", "4"},
    {"--2^2", "2^2"},
    {"---2^2", "(-2)^2"},
    {"-2^2", "4"},
    {"2^1.1^1.2^1.3", "((2^1.1)^1.2)^1.3"},
    {"-a^b", "(-a)^b"},
    {"-a^-b", "(-a)^(-b)"},
    {"1^0", "1"},
    {"(1)^0", "1"},
    {"(-1)^0", "1"},
    {"(-5)^0", "1"},
    {"(!0)^0", "1"},
    {"(0)^0", "1"},
    {"(!!5)^0", "1"},
    {"2^-3^4", "(2^(-3))^4"},
    {"-2^-3^-4", "((-2)^(-3))^(-4)"}
};

void test_pow() {
    double a, b;
    te_variable lookup[2];
    int i, case_count;
    double r1, r2;
    const char *expr1, *expr2;
    te_expr *ex1, *ex2;
    int olfail;
    test_equ *cases;
    a = 2; b = 3;

    cases = te_pow_cases_left;
    case_count = (int)(sizeof(te_pow_cases_left) / sizeof(te_pow_cases_left[0]));

    lookup[0].name = "a"; lookup[0].address = &a; lookup[0].type = TE_VARIABLE; lookup[0].context = 0;
    lookup[1].name = "b"; lookup[1].address = &b; lookup[1].type = TE_VARIABLE; lookup[1].context = 0;

    for (i = 0; i < case_count; ++i) {
        expr1 = cases[i].expr1;
        expr2 = cases[i].expr2;
        ex1 = te_compile(expr1, lookup, sizeof(lookup)/sizeof(te_variable), 0);
        ex2 = te_compile(expr2, lookup, sizeof(lookup)/sizeof(te_variable), 0);

        lok(ex1);
        lok(ex2);

        r1 = te_eval(ex1);
        r2 = te_eval(ex2);

        fflush(stdout);
        olfail = lfails;
        lfequal(r1, r2);
        if (olfail != lfails) {
            printf("Failed expression: %s <> %s (%f <> %f)\n", expr1, expr2, r1, r2);
        }

        te_free(ex1);
        te_free(ex2);
    }

}

void test_combinatorics() {
    int i;
    for (i = 0; i < sizeof(smoke_combinatorics_cases) / sizeof(test_case); ++i) {
        const char *expr = smoke_combinatorics_cases[i].expr;
        const double answer = smoke_combinatorics_cases[i].answer;
        /* Above 2^53 a double cannot hold every integer, so allow a few ulps. */
        const double tol = answer > 9007199254740992.0 ? answer * 1e-15 : LTEST_FLOAT_TOLERANCE;

        int err;
        const double ev = te_interp(expr, &err);
        lok(!err);
        lok(fabs(ev - answer) <= tol);

        if (err || fabs(ev - answer) > tol) {
            printf("FAILED: %s => %f, expected %f (err=%d)\n", expr, ev, answer, err);
        }
    }
}


/* These quotients are exact, so the modf()-based te_fmod gives them exactly. */
static test_case smoke_modulus_cases[] = {
        {"5.5%2", 1.5}, {"-5.5%2", -1.5}, {"7%-3", 1}, {"-7%3", -1},
        {"6%3", 0}, {"5%0.5", 0}, {"0%3", 0}
};

void test_modulus() {
    int i;
    for (i = 0; i < sizeof(smoke_modulus_cases) / sizeof(test_case); ++i) {
        const char *expr = smoke_modulus_cases[i].expr;
        const double answer = smoke_modulus_cases[i].answer;

        int err;
        const double ev = te_interp(expr, &err);
        lok(!err);
        lok(fabs(ev - answer) <= LTEST_FLOAT_TOLERANCE);

        if (err || fabs(ev - answer) > LTEST_FLOAT_TOLERANCE)
            printf("FAILED: %s => %f, expected %f (err=%d)\n", expr, ev, answer, err);
    }

    /* 1%0.1 is 0.09999999999999995 with fmod(), but the modf() quotient can
       round up to 10 and give 0 on Aztec. Both must lie in the remainder range. */
    {
        int err;
        const double pos = te_interp("1%0.1", &err);
        const double neg = te_interp("-1%0.1", &err);
        lok(pos >= 0.0 && pos <= 0.1);
        lok(neg <= 0.0 && neg >= -0.1);
        if (pos < 0.0 || pos > 0.1 || neg > 0.0 || neg < -0.1)
            printf("FAILED: 1%%0.1 => %f, -1%%0.1 => %f\n", pos, neg);
    }
}

void test_logic() {
    int i;
    for (i = 0; i < sizeof(smoke_logic_cases) / sizeof(test_case); ++i) {
        const char *expr = smoke_logic_cases[i].expr;
        const double answer = smoke_logic_cases[i].answer;

        int err;
        const double ev = te_interp(expr, &err);
        lok(!err);
        lfequal(ev, answer);

        if (err) {
            printf("FAILED: %s (%d)\n", expr, err);
        }
    }
}


void test_depth() {
    /* Expressions nested deeper than TE_MAX_DEPTH must fail cleanly
     * instead of overflowing the stack. */
    
    int i;
    for (i = 0; i < (int)(sizeof(smoke_depths) / sizeof(int)); ++i) {
        const int depth = smoke_depths[i];
        const int ok = depth < 500;
        int j, err;
        int r;

        /* ((((...1...)))) */
        char *expr = malloc(depth * 2 + 2);
        if (!expr) { lok(0); continue; }
        memset(expr, '(', depth);
        expr[depth] = '1';
        memset(expr + depth + 1, ')', depth);
        expr[depth * 2 + 1] = '\0';

        r = te_interp(expr, &err);
        lok(ok ? (err == 0 && r == 1.0) : (err != 0 && r != r));
        free(expr);

        /* sin sin sin ... 1 */
        expr = malloc(depth * 4 + 2);
        if (!expr) { lok(0); continue; }
        for (j = 0; j < depth; ++j) memcpy(expr + j * 4, "sin ", 4);
        expr[depth * 4] = '1';
        expr[depth * 4 + 1] = '\0';

        r = te_interp(expr, &err);
        lok(ok ? (err == 0) : (err != 0 && r != r));
        free(expr);
    }
}


static unsigned long number_rand_state = 1UL;

static unsigned long number_rand(unsigned long modulus) {
    /* 32-bit unsigned arithmetic is available on the DOS target and wraps. */
    number_rand_state = number_rand_state * 1103515245UL + 12345UL;
    return (unsigned long)((number_rand_state >> 16) % modulus);
}


void test_number() {
    /* Number parsing goes through te_strtod, which is the reference here.
     * The fuzz checks that te_interp's lexer agrees with it on random input. */
    int i;
    for (i = 0; i < 5000; ++i) {
        char expr[64];
        char *p = expr;
        int j;

        if (number_rand(8) == 0) {
            /* Hex constant. */
            const int digits = 1 + number_rand(8);
            p += sprintf(p, "0x");
            for (j = 0; j < digits; ++j)
                p += sprintf(p, "%c", "0123456789abcdefABCDEF"[number_rand(22)]);
        } else {
            const int int_digits = number_rand(11);
            const int frac_digits = int_digits ? number_rand(11) : 1 + number_rand(10);

            for (j = 0; j < int_digits; ++j)
                p += sprintf(p, "%c", "0123456789"[number_rand(10)]);
            if (frac_digits || number_rand(2)) {
                p += sprintf(p, ".");
                for (j = 0; j < frac_digits; ++j)
                    p += sprintf(p, "%c", "0123456789"[number_rand(10)]);
            }
            if (number_rand(2)) {
                p += sprintf(p, "%c", "eE"[number_rand(2)]);
                if (number_rand(2)) p += sprintf(p, "%c", "+-"[number_rand(2)]);
                p += sprintf(p, "%lu", number_rand(320));
            }
        }

        {
            const double expected = te_strtod(expr, 0);
            int err;
            const double got = te_interp(expr, &err);
            /* The small absolute slack covers subnormals, where repeated
             * scaling can differ from te_strtod by more than a ulp. */
            const int ok = err == 0 && (got == expected ||
                fabs(got - expected) <= 1e-14 * fabs(expected) + 1e-304);
            lok(ok);
            if (!ok) printf("FAILED: %s => %.17g, expected %.17g (err=%d)\n",
                expr, got, expected, err);
        }
    }

    /* Forms that stop being a number partway through must still parse the
     * same way: "1e" is the number 1 times nothing (syntax error at "e"),
     * "0x" is the number 0 then the unknown variable x. */
    {
        
        int j;
        for (j = 0; j < (int)(sizeof(smoke_partial_number_exprs) / sizeof(*smoke_partial_number_exprs)); ++j) {
            int err;
            const double r = te_interp(smoke_partial_number_exprs[j], &err);
            lok(is_error_result(r));
        }
    }

    /* Short decimal literals (up to 15 significant digits) must match
     * te_strtod exactly, not just within the fuzz slack above. Scaling by an
     * inexact pow(10, -n) made these one ulp off on native targets. Whether
     * 0.1+0.2==0.3 holds depends on the platform's double arithmetic (it is
     * false on IEEE hosts and true on Aztec), so compare against the C result. */
    {
        
        int j;
        for (j = 0; j < (int)(sizeof(smoke_exact_number_exprs) / sizeof(*smoke_exact_number_exprs)); ++j) {
            int err;
            const double expected = te_strtod(smoke_exact_number_exprs[j], 0);
            const double got = te_interp(smoke_exact_number_exprs[j], &err);
            lok(err == 0 && got == expected);
            if (err != 0 || got != expected)
                printf("FAILED: %s => %.17g, expected %.17g (err=%d)\n",
                    smoke_exact_number_exprs[j], got, expected, err);
        }
        lok(te_interp("0.1+0.2==0.3", 0) == (double)(0.1 + 0.2 == 0.3));
        lok(te_interp("0.1+0.2", 0) == 0.1 + 0.2);
    }
}

int main(int argc, char *argv[])
{
    lrun("Results", test_results);
    lrun("Syntax", test_syntax);
    lrun("NaNs", test_nans);
    lrun("INFs", test_infs);
    lrun("Variables", test_variables);
    lrun("Functions", test_functions);
    lrun("Dynamic", test_dynamic);
    lrun("Closure", test_closure);
    lrun("Optimize", test_optimize);
    lrun("Pow", test_pow);
    lrun("Combinatorics", test_combinatorics);
    lrun("Modulus", test_modulus);
    lrun("Logic", test_logic);
    lrun("Depth", test_depth);
    lrun("Number", test_number);
    lresults();

    return lfails != 0;
}

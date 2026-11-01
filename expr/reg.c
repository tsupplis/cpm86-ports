#include <math.h>
#include <stdio.h>

/* Volatile operands exercise the runtime, not compiler constant folding. */
static volatile double five = 5.0, zero = 0.0;

int main(void) {
    double power = pow(five, zero);
    double infinity = five / zero;
    double invalid = zero / zero;
    printf("pow(5,0)=%f\n", power);
    printf("5/0=%f\n", infinity);
    printf("0/0=%f\n", invalid);
#ifdef REQUIRE_INFINITY
    if (power != 1.0 || !isinf(infinity) || !isnan(invalid)
        || !isinf(log(zero)) || !(log(zero) < 0.0)) {
        puts("MATH EDGE FAIL");
        return 1;
    }
    puts("MATH EDGE PASS");
#endif
    return 0;
}

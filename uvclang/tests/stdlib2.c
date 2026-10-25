// Differential test for div()/ldiv()/lldiv(): uvclang's UVM-side stdlib vs
// the platform's native libc. Both must agree on stdout and exit code.
//
// div_t (2 ints, 8 bytes) and ldiv_t/lldiv_t (2 longs/long longs, 16 bytes)
// are returned by value and hit different ABI coercion shapes (a plain i64
// vs. an LLVM [2 x i64] array), so each flavor is checked independently:
// the exit code is a per-flavor pass count, so a regression in just one of
// the three shows up as a smaller number instead of a single pass/fail bit.
// Cases cover exact division, a nonzero remainder, every sign combination
// (truncation toward zero; remainder takes the dividend's sign), zero
// dividend, divisor == 1, divisor == dividend, and (lldiv only) an operand
// that doesn't fit in 32 bits. INT_MIN/-1 is UB and is never exercised.
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

static int approx(float a, float b) { return fabsf(a - b) < 1e-3f; }
static int approxd(double a, double b) { return fabsf(a - b) < 1e-3f; }

int test_str_to_float(int r)
{

    r += (approx(strtof("5.0", NULL), 5.0));
    r += (approx(strtof("7845.0", NULL), 7845.0));
    r += (approx(strtof("-771.0", NULL), -771.0));
    r += (approx(strtof("-771.83", NULL), -771.83));
    r += (approx(strtof("8.4E2", NULL), 840.0));
    r += (approx(strtof("-8.4E2", NULL), -840.0));
    r += (approx(strtof("8.4E-2", NULL), 0.084));
    r += (approx(strtof("-0x1afp-2", NULL), -107.75));
    r += (approx(strtof("0X1.BC70A3D70A3D7P+6", NULL), 111.11));

    return r;
}

int test_str_to_double(int r)
{
    r += (approxd(strtod("5.0", NULL), 5.0));
    r += (approxd(strtod("7845.0", NULL), 7845.0));
    r += (approxd(strtod("-771.0", NULL), -771.0));
    r += (approxd(strtod("-771.83", NULL), -771.83));
    r += (approxd(strtod("8.4E2", NULL), 840.0));
    r += (approxd(strtod("-8.4E2", NULL), -840.0));
    r += (approxd(strtod("8.4E-2", NULL), 0.084));
    r += (approxd(strtod("-0x1afp-2", NULL), -107.75));
    r += (approxd(strtod("0X1.BC70A3D70A3D7P+6", NULL), 111.11));

    r += (approxd(strtod("1E300", NULL), 1E300));

    return r;
}

int test_div(int r)
{
    div_t d;
    d = div(1332, 37);  r += (d.quot == 36  && d.rem == 0);   // exact
    d = div(7, 2);       r += (d.quot == 3   && d.rem == 1);   // +/+ remainder
    d = div(-7, 2);      r += (d.quot == -3  && d.rem == -1);  // -/+
    d = div(7, -2);      r += (d.quot == -3  && d.rem == 1);   // +/-
    d = div(-7, -2);     r += (d.quot == 3   && d.rem == -1);  // -/-
    d = div(0, 5);       r += (d.quot == 0   && d.rem == 0);   // zero dividend
    d = div(5, 1);       r += (d.quot == 5   && d.rem == 0);   // divisor == 1
    d = div(5, 5);       r += (d.quot == 1   && d.rem == 0);   // divisor == dividend
    printf("div = %d,%d\n", d.quot, d.rem);

    ldiv_t ld;
    ld = ldiv(1332L, 37L);   r += (ld.quot == 36  && ld.rem == 0);
    ld = ldiv(7L, 2L);       r += (ld.quot == 3   && ld.rem == 1);
    ld = ldiv(-7L, 2L);      r += (ld.quot == -3  && ld.rem == -1);
    ld = ldiv(7L, -2L);      r += (ld.quot == -3  && ld.rem == 1);
    ld = ldiv(-7L, -2L);     r += (ld.quot == 3   && ld.rem == -1);
    ld = ldiv(0L, 5L);       r += (ld.quot == 0   && ld.rem == 0);
    ld = ldiv(5L, 1L);       r += (ld.quot == 5   && ld.rem == 0);
    ld = ldiv(5L, 5L);       r += (ld.quot == 1   && ld.rem == 0);
    printf("ldiv = %ld,%ld\n", ld.quot, ld.rem);

    lldiv_t lld;
    lld = lldiv(1332LL, 37LL);   r += (lld.quot == 36  && lld.rem == 0);
    lld = lldiv(7LL, 2LL);       r += (lld.quot == 3   && lld.rem == 1);
    lld = lldiv(-7LL, 2LL);      r += (lld.quot == -3  && lld.rem == -1);
    lld = lldiv(7LL, -2LL);      r += (lld.quot == -3  && lld.rem == 1);
    lld = lldiv(-7LL, -2LL);     r += (lld.quot == 3   && lld.rem == -1);
    lld = lldiv(0LL, 5LL);       r += (lld.quot == 0   && lld.rem == 0);
    lld = lldiv(5LL, 1LL);       r += (lld.quot == 5   && lld.rem == 0);
    lld = lldiv(5LL, 5LL);       r += (lld.quot == 1   && lld.rem == 0);
    lld = lldiv(5000000000LL, 7LL);
    r += (lld.quot == 714285714LL && lld.rem == 2LL);          // > 32-bit operand
    // printf("lldiv = %lld,%lld\n", lld.quot, lld.rem);

    return r;   
}


int main()
{
    int r = 0;

    r = test_div(r);
    r = test_str_to_float(r);
    r = test_str_to_double(r);

    return r;   // exit code == number of passing checks (44 total)
}